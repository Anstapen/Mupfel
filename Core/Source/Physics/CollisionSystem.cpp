#include "CollisionSystem.h"
#include "Core/Application.h"
#include "Core/Profiler.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <thread>

/* Needed Component types for collision detection/resolution */
#include "ECS/Components/Body.h"
#include "ECS/Components/Collider.h"
#include "ECS/Components/Sensor.h"
#include "ECS/Components/Transform.h"

#include "Core/PhysicsEvents.h"

using namespace Mupfel;

Mupfel::CollisionSystem::CollisionSystem(Registry& reg, EventSystem& evt_sys)
	: registry(reg), evt_system(evt_sys), entity_has_events(100, false)
{
}

void CollisionSystem::Init()
{
	logger = Logger::Create("Collision System");
	evt_system.RegisterListener<ComponentAddedEvent>(
		[this](const ComponentAddedEvent& ev)
		{
			/* The entity needs a transform and body component. */
			static const Entity::Signature required_for_body = Registry::ComponentSignature<Transform, Body>();

			/*
			 * The component added also needs to be one of those. Basically, we only care about the case where the given
			 * component "completes" the signature.
			 */
			const bool for_body =
				ev.comp_id == ComponentIndex::Index<Transform>() || ev.comp_id == ComponentIndex::Index<Body>();

			if (for_body && ((ev.sig & required_for_body) == required_for_body) && !HasBody(ev.e))
			{
				/* The entity has all required components for body creation. */
				std::scoped_lock lock(pending_mutex);
				pending_body_create.push(ev.e);
			}

			static const Entity::Signature required_for_collider =
				Registry::ComponentSignature<Transform, Body, Collider>();
			const bool for_collider = for_body || ev.comp_id == ComponentIndex::Index<Collider>();

			if (for_collider && (ev.sig & required_for_collider) == required_for_collider)
			{
				/* The entity has all required components for collider creation. */
				std::scoped_lock lock(pending_mutex);
				pending_collider_create.push(ev.e);
			}

			static const Entity::Signature required_for_sensor =
				Registry::ComponentSignature<Transform, Body, Sensor>();
			const bool for_sensor = for_body || ev.comp_id == ComponentIndex::Index<Sensor>();

			if (for_sensor && (ev.sig & required_for_sensor) == required_for_sensor)
			{
				/* The entity has all required components for sensor creation. */
				std::scoped_lock lock(pending_mutex);
				pending_sensor_create.push(ev.e);
			}
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
	{
		HandlePendingEvents();
	}
	{
		b2World_Step(current_world, static_cast<float>(delta), sub_steps);
	}
}

void Mupfel::CollisionSystem::SceneSwitched(SceneHandle new_scene, float grav_x, float grav_y)
{
	current_world = WorldForScene(new_scene);
	b2World_SetGravity(current_world, {grav_x, grav_y});
}

void CollisionSystem::SyncTransforms()
{
	assert(!B2_IS_NULL(current_world));
	b2BodyEvents events = b2World_GetBodyEvents(current_world);
	auto&		 transforms = registry.GetComponentArray<Transform>();

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
	}
}

void CollisionSystem::DispatchEvents()
{
	assert(!B2_IS_NULL(current_world));

	/* Clear the entity event buffer */
	std::fill(entity_has_events.begin(), entity_has_events.end(), false);

	b2ContactEvents contacts = b2World_GetContactEvents(current_world);
	for (int i = 0; i < contacts.beginCount; ++i)
	{
		const b2ContactBeginTouchEvent& ev = contacts.beginEvents[i];
		Entity							a = EntityOf(ev.shapeIdA);
		Entity							b = EntityOf(ev.shapeIdB);
		SetEventForEntity(a);
		SetEventForEntity(b);
		evt_system.AddEvent<CollisionBeganEvent>({a, b});
	}

	for (int i = 0; i < contacts.endCount; ++i)
	{
		const b2ContactEndTouchEvent& ev = contacts.endEvents[i];

		if (!b2Shape_IsValid(ev.shapeIdA) || !b2Shape_IsValid(ev.shapeIdB))
		{
			continue;
		}

		Entity a = EntityOf(ev.shapeIdA);
		Entity b = EntityOf(ev.shapeIdB);
		SetEventForEntity(a);
		SetEventForEntity(b);

		evt_system.AddEvent<CollisionEndedEvent>({a, b});
	}

	for (int i = 0; i < contacts.hitCount; ++i)
	{
		const b2ContactHitEvent& ev = contacts.hitEvents[i];

		Entity a = EntityOf(ev.shapeIdA);
		Entity b = EntityOf(ev.shapeIdB);
		SetEventForEntity(a);
		SetEventForEntity(b);

		evt_system.AddEvent<CollisionHitEvent>({a, b});
	}

	b2SensorEvents sensors = b2World_GetSensorEvents(current_world);
	for (int i = 0; i < sensors.beginCount; ++i)
	{
		const b2SensorBeginTouchEvent& ev = sensors.beginEvents[i];

		Entity sensor = EntityOf(ev.sensorShapeId);
		Entity visitor = EntityOf(ev.visitorShapeId);
		SetEventForEntity(sensor);
		SetEventForEntity(visitor);

		evt_system.AddEvent<SensorEnteredEvent>({sensor, visitor});
	}

	for (int i = 0; i < sensors.endCount; ++i)
	{
		const b2SensorEndTouchEvent& ev = sensors.endEvents[i];

		if (!b2Shape_IsValid(ev.sensorShapeId) || !b2Shape_IsValid(ev.visitorShapeId))
		{
			continue;
		}

		Entity sensor = EntityOf(ev.sensorShapeId);
		Entity visitor = EntityOf(ev.visitorShapeId);
		SetEventForEntity(sensor);
		SetEventForEntity(visitor);

		evt_system.AddEvent<SensorExitedEvent>({sensor, visitor});
	}
}

void Mupfel::CollisionSystem::DeInit()
{
	for (auto& id : worlds)
	{
		b2DestroyWorld(id.second);
	}
}

void Mupfel::CollisionSystem::SetTransform(Entity e, Transform t)
{
	/* For now, we silently just do no nothing if the entity does not have a body or a transform component. */
	if (HasBody(e))
	{
		b2Body_SetTransform(bodies[e.Index()], {t.pos_x, t.pos_y}, b2MakeRot(t.rotation * (B2_PI / 180.0f)));
		b2Body_SetAwake(bodies[e.Index()], true);
	}

	Application::GetCurrentRegistry().AddComponent<Transform>(e, t);
}

void Mupfel::CollisionSystem::SetMovement(Entity e, float vel_x, float vel_y, float vel_ang)
{
	/* Same strategy as for SetTransform. */
	if (HasBody(e))
	{
		b2Body_SetLinearVelocity(bodies[e.Index()], {vel_x, vel_y});
		b2Body_SetAngularVelocity(bodies[e.Index()], vel_ang);
		b2Body_SetAwake(bodies[e.Index()], true);
	}
}

bool Mupfel::CollisionSystem::HasEvents(Entity e)
{
	if (e.Index() >= entity_has_events.size())
	{
		return false;
	}

	return entity_has_events[e.Index()];
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

	while (!pending_body_create.empty())
	{
		{
			std::scoped_lock lock(pending_mutex);

			CreateBody(pending_body_create.front());
			pending_body_create.pop();
		}
	}

	while (!pending_collider_create.empty())
	{
		{
			std::scoped_lock lock(pending_mutex);

			CreateCollider(pending_collider_create.front());
			pending_collider_create.pop();
		}
	}

	while (!pending_sensor_create.empty())
	{
		{
			std::scoped_lock lock(pending_mutex);

			CreateSensor(pending_sensor_create.front());
			pending_sensor_create.pop();
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

	const SceneHandle scene = Scene::HandleFromMask(registry.GetSceneMask(e));
	if (scene >= Scene::MAX_SCENES)
	{
		return;
	}

	/* The entity needs a transform and body component. */
	if (!registry.HasComponent<Transform>(e) || !registry.HasComponent<Body>(e))
	{
		return;
	}

	const Transform& t = registry.GetComponent<Transform>(e);
	const Body&		 b = registry.GetComponent<Body>(e);

	b2BodyDef def = b2DefaultBodyDef();
	switch (b.type)
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
	def.gravityScale = b.gravity_scale;
	def.linearDamping = b.linear_damping;
	def.angularDamping = b.angular_damping;
	def.fixedRotation = b.fixed_rotation;
	def.isBullet = b.is_bullet;
	def.enableSleep = b.allow_sleep;
	def.angularVelocity = b.angular_velocity;
	def.linearVelocity.x = b.velocity_x;
	def.linearVelocity.y = b.velocity_y;
	def.userData = Registry::ToUserData(e);

	b2WorldId world_to_use = WorldForScene(scene);
	b2BodyId  body = b2CreateBody(world_to_use, &def);

	SetBody(e, body);
}

void Mupfel::CollisionSystem::CreateCollider(Entity e)
{
	/* If the entity currently does not have a body, there is nothing to do. */
	if (!HasBody(e))
	{
		return;
	}

	assert(bodies.size() > e.Index());
	assert(!B2_IS_NULL(current_world));

	/* We need to be in a valid scene at the moment. */
	const SceneHandle scene = Scene::HandleFromMask(registry.GetSceneMask(e));
	if (scene >= Scene::MAX_SCENES)
	{
		return;
	}

	/* The entity needs all three components to create a valid collider. */
	if (!registry.HasComponent<Transform>(e) || !registry.HasComponent<Body>(e) || !registry.HasComponent<Collider>(e))
	{
		return;
	}

	const Collider&	 c = registry.GetComponent<Collider>(e);

	b2BodyId body = bodies[e.Index()];

	b2ShapeDef sd = b2DefaultShapeDef();
	sd.density = c.density;
	sd.material.friction = c.friction;
	sd.material.restitution = c.restitution;
	sd.isSensor = false;
	sd.enableContactEvents = c.report_contacts;
	sd.enableHitEvents = c.report_hit_events;
	sd.enableSensorEvents = c.report_sensor_events;
	sd.filter.categoryBits = c.category;
	sd.filter.maskBits = c.mask;
	sd.userData = Registry::ToUserData(e);

	switch (c.shape)
	{
	case ColliderShape::Box:
	{
		b2Polygon box =
			b2MakeOffsetBox(c.data.box.half_width, c.data.box.half_height, {c.offset_x, c.offset_y}, b2Rot_identity);
		b2CreatePolygonShape(body, &sd, &box);
		break;
	}
	case ColliderShape::Circle:
	{
		b2Circle circle = {{c.offset_x, c.offset_y}, c.data.circle.radius};
		b2CreateCircleShape(body, &sd, &circle);
		break;
	}
	case ColliderShape::Capsule: /* b2Capsule + b2CreateCapsuleShape */
		b2Capsule capsule = {
			{c.data.capsule.center1_x + c.offset_x, c.data.capsule.center1_y + c.offset_y},
			{c.data.capsule.center2_x + c.offset_x, c.data.capsule.center2_y + c.offset_y},
			c.data.capsule.radius};
		b2CreateCapsuleShape(body, &sd, &capsule);
		break;
	}
}

void Mupfel::CollisionSystem::CreateSensor(Entity e)
{
	/* If the entity currently does not have a body, there is nothing to do. */
	if (!HasBody(e))
	{
		return;
	}

	assert(bodies.size() > e.Index());
	assert(!B2_IS_NULL(current_world));

	/* We need to be in a valid scene at the moment. */
	const SceneHandle scene = Scene::HandleFromMask(registry.GetSceneMask(e));
	if (scene >= Scene::MAX_SCENES)
	{
		return;
	}

	/* The entity needs all three components to create a valid sensor. */
	if (!registry.HasComponent<Transform>(e) || !registry.HasComponent<Body>(e) || !registry.HasComponent<Sensor>(e))
	{
		return;
	}

	const Sensor&	 s = registry.GetComponent<Sensor>(e);

	b2BodyId body = bodies[e.Index()];

	b2ShapeDef sd = b2DefaultShapeDef();
	sd.isSensor = true;
	sd.enableContactEvents = false;
	sd.enableHitEvents = false;
	sd.enableSensorEvents = s.report_events;
	sd.filter.categoryBits = s.category;
	sd.filter.maskBits = s.mask;
	sd.userData = Registry::ToUserData(e);

	switch (s.shape)
	{
	case ColliderShape::Box:
	{
		b2Polygon box =
			b2MakeOffsetBox(s.data.box.half_width, s.data.box.half_height, {s.offset_x, s.offset_y}, b2Rot_identity);
		b2CreatePolygonShape(body, &sd, &box);
		break;
	}
	case ColliderShape::Circle:
	{
		b2Circle circle = {{s.offset_x, s.offset_y}, s.data.circle.radius};
		b2CreateCircleShape(body, &sd, &circle);
		break;
	}
	case ColliderShape::Capsule:
		b2Capsule capsule = {
			{s.data.capsule.center1_x, s.data.capsule.center1_y},
			{s.data.capsule.center2_x, s.data.capsule.center2_y},
			s.data.capsule.radius};
		b2CreateCapsuleShape(body, &sd, &capsule);
		break;
	}
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

void Mupfel::CollisionSystem::SetEventForEntity(Entity e)
{
	if (e.Index() >= entity_has_events.size())
	{
		uint64_t new_size = (static_cast<uint64_t>(e.Index()) * 2);
		new_size = std::min<uint64_t>(std::numeric_limits<uint32_t>::max(), new_size);
		entity_has_events.resize(new_size, false);
	}

	entity_has_events[e.Index()] = true;
}
