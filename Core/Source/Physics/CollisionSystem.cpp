#include "CollisionSystem.h"
#include "Core/Application.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <thread>

/* Needed Component types for collision detection/resolution */
#include "ECS/Components/Collider.h"
#include "ECS/Components/Movement.h"
#include "ECS/Components/RigidBody.h"
#include "ECS/Components/Transform.h"

#include "Core/PhysicsEvents.h"

using namespace Mupfel;

Mupfel::CollisionSystem::CollisionSystem(Registry& reg, EventSystem& evt_sys) : registry(reg), evt_system(evt_sys) {}

void CollisionSystem::Init()
{
	evt_system.RegisterListener<ComponentAddedEvent>(
		[this](const ComponentAddedEvent& ev)
		{
			static const Entity::Signature required = Registry::ComponentSignature<Transform, RigidBody, Collider>();

			if ((ev.sig & required) != required)
				return;
			if (HasBody(ev.e))
				return;

			std::scoped_lock lock(pending_mutex);
			pending_create.push(ev.e);
		});

	evt_system.RegisterListener<EntityDestroyedEvent>(
		[this](const EntityDestroyedEvent& ev)
		{
			b2BodyId id = TakeBody(ev.e);
			if (B2_IS_NON_NULL(id))
			{
				std::scoped_lock lock(pending_mutex);
				pending_destroy.push(id);
			}
		});
}

void CollisionSystem::Step(double delta, uint32_t sub_steps)
{
	assert(!B2_IS_NULL(current_world));
	HandlePendingEvents();
	b2World_Step(current_world, delta, sub_steps);
}

void Mupfel::CollisionSystem::SceneSwitched(SceneHandle new_scene) { current_world = WorldForScene(new_scene); }

void CollisionSystem::SyncTransforms()
{
	assert(!B2_IS_NULL(current_world));
	b2BodyEvents events = b2World_GetBodyEvents(current_world);
	auto&		 transforms = registry.GetComponentArray<Transform>();
	auto&		 movements = registry.GetComponentArray<Movement>();

	for (int i = 0; i < events.moveCount; ++i)
	{
		const b2BodyMoveEvent& ev = events.moveEvents[i];

		Entity e = Registry::EntityFromIndex(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ev.userData)));

		if (transforms.Has(e))
		{
			Transform& t = transforms.Get(e);
			t.pos_x = ev.transform.p.x;
			t.pos_y = ev.transform.p.y;
			t.rotation = b2Rot_GetAngle(ev.transform.q);
		}

		if (movements.Has(e))
		{
			Movement& t = movements.Get(e);
			b2Vec2	  velocity = b2Body_GetLinearVelocity(ev.bodyId);
			t.angular_velocity = b2Body_GetAngularVelocity(ev.bodyId);
			t.velocity_x = velocity.x;
			t.velocity_y = velocity.y;
		}
	}
}

void CollisionSystem::DispatchEvents()
{
	assert(!B2_IS_NULL(current_world));
	b2ContactEvents contacts = b2World_GetContactEvents(current_world);
	for (int i = 0; i < contacts.beginCount; ++i)
	{
		const b2ContactBeginTouchEvent& ev = contacts.beginEvents[i];
		evt_system.AddEvent<CollisionBeganEvent>({EntityOf(ev.shapeIdA), EntityOf(ev.shapeIdB)});
	}
	// end events: shapes MAY already be destroyed -- guard with b2Shape_IsValid (types.h:1070)
	// sensor events: b2World_GetSensorEvents, same shape.
}

void Mupfel::CollisionSystem::DeInit()
{
	for (auto& id : worlds)
	{
		b2DestroyWorld(id.second);
	}
}

void Mupfel::CollisionSystem::SetTransform(Entity e, Transform&& t)
{
	/* For now, we silently just do no nothing if the entity does not have a body or a transform component. */
	if (HasBody(e))
	{
		b2Body_SetTransform(bodies[e.Index()], {t.pos_x, t.pos_y}, b2MakeRot(t.rotation * (B2_PI / 180.0f)));
	}

	Application::GetCurrentRegistry().AddComponent<Transform>(e, t);
}

void Mupfel::CollisionSystem::SetMovement(Entity e, Movement&& m)
{ 
	/* Same strategy as for SetTransform. */
	if (HasBody(e))
	{
		b2Body_SetLinearVelocity(bodies[e.Index()], {m.velocity_x, m.velocity_y});
		b2Body_SetAngularVelocity(bodies[e.Index()], m.angular_velocity);
	}

	Application::GetCurrentRegistry().AddComponent<Movement>(e, m);
}

bool Mupfel::CollisionSystem::HasBody(Entity e) const
{
	return (e.Index() < bodies.size()) && (!B2_ID_EQUALS(bodies[e.Index()], b2_nullBodyId));
}

void Mupfel::CollisionSystem::HandlePendingEvents()
{
	while (!pending_destroy.empty())
	{
		b2BodyId id;
		{
			std::scoped_lock lock(pending_mutex);

			id = pending_destroy.front();
			pending_destroy.pop();
		}

		b2DestroyBody(id);
	}

	while (!pending_create.empty())
	{
		{
			std::scoped_lock lock(pending_mutex);

			CreateBody(pending_create.front());
			pending_create.pop();
		}
	}
}

void CollisionSystem::CreateBody(Entity e)
{
	if (HasBody(e))
	{
		return;
	}

	assert(!B2_IS_NULL(current_world));

	/* If the entity was destroyed in the meantime, do not create the body! */
	const SceneHandle scene = Scene::HandleFromMask(registry.GetSceneMask(e));
	if (scene >= Scene::MAX_SCENES)
	{
		return;
	}

	const Transform& t = registry.GetComponent<Transform>(e);
	const RigidBody& rb = registry.GetComponent<RigidBody>(e);
	const Collider&	 c = registry.GetComponent<Collider>(e);

	b2BodyDef def = b2DefaultBodyDef();
	switch (rb.type)
	{
	case BodyType::Dynamic:
		def.type = b2_dynamicBody;
		break;
	case BodyType::Kinematic:
		def.type = b2_kinematicBody;
		break;
	default:
		def.type = b2_staticBody;
		break;
	}
	def.position = {t.pos_x, t.pos_y};
	def.rotation = b2MakeRot(t.rotation);
	def.gravityScale = rb.gravity_scale;
	def.linearDamping = rb.linear_damping;
	def.angularDamping = rb.angular_damping;
	def.fixedRotation = rb.fixed_rotation;
	def.isBullet = rb.is_bullet;
	def.enableSleep = rb.allow_sleep;
	def.userData = Registry::ToUserData(e);

	if (registry.HasComponent<Movement>(e))
	{
		Movement m = registry.GetComponent<Movement>(e);
		def.angularVelocity = m.angular_velocity;
		def.linearVelocity.x = m.velocity_x;
		def.linearVelocity.y = m.velocity_y;
	}

	b2WorldId world_to_use = WorldForScene(scene);
	b2BodyId  body = b2CreateBody(world_to_use, &def);

	b2ShapeDef sd = b2DefaultShapeDef();
	sd.density = c.density;
	sd.material.friction = c.friction;
	sd.material.restitution = c.restitution;
	sd.isSensor = c.is_sensor;
	sd.enableContactEvents = c.report_contacts;
	sd.enableSensorEvents = true;
	sd.filter.categoryBits = c.category;
	sd.filter.maskBits = c.mask;
	sd.userData = Registry::ToUserData(e);

	switch (c.shape)
	{
	case ColliderShape::Box:
	{
		b2Polygon box = b2MakeOffsetBox(c.half_width, c.half_height, {c.offset_x, c.offset_y}, b2Rot_identity);
		b2CreatePolygonShape(body, &sd, &box);
		break;
	}
	case ColliderShape::Circle:
	{
		b2Circle circle = {{c.offset_x, c.offset_y}, c.half_width};
		b2CreateCircleShape(body, &sd, &circle);
		break;
	}
	case ColliderShape::Capsule: /* b2Capsule + b2CreateCapsuleShape */
		break;
	}

	SetBody(e, body);
}

b2BodyId Mupfel::CollisionSystem::TakeBody(Entity e)
{
	if (e.Index() >= bodies.size() || B2_ID_EQUALS(bodies[e.Index()], b2_nullBodyId))
	{
		return b2_nullBodyId;
	}

	b2BodyId body = bodies[e.Index()];
	bodies[e.Index()] = b2_nullBodyId;

	return body;
}

void Mupfel::CollisionSystem::SetBody(Entity e, b2BodyId body)
{
	if (e.Index() >= bodies.size())
	{
		uint64_t new_size = ((static_cast<uint64_t>(e.Index()) + 1) * 2);
		new_size = std::min<uint64_t>(std::numeric_limits<uint32_t>::max(), new_size);
		bodies.resize(new_size, b2_nullBodyId);
	}

	assert(B2_ID_EQUALS(bodies[e.Index()], b2_nullBodyId) && "Entity already has a Body!");

	bodies[e.Index()] = body;
}

Entity Mupfel::CollisionSystem::EntityOf(b2ShapeId id)
{
	return Registry::EntityFromIndex(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(b2Shape_GetUserData(id))));
}

Entity Mupfel::CollisionSystem::EntityOf(b2BodyId id)
{
	return Registry::EntityFromIndex(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(id))));
}

b2WorldId Mupfel::CollisionSystem::WorldForScene(SceneHandle scene)
{
	auto [it, inserted] = worlds.try_emplace(scene, b2_nullWorldId);
	if (inserted)
	{
		b2WorldDef def = b2DefaultWorldDef();
		def.gravity = {0.0f, 0.0f};
		it->second = b2CreateWorld(&def);
	}
	return it->second;
}
