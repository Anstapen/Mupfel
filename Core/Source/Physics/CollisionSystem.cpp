#include "CollisionSystem.h"
#include "Core/Application.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <thread>

/* Needed Component types for collision detection/resolution */
#include "ECS/Components/Collider.h"
#include "ECS/Components/Transform.h"

using namespace Mupfel;

Mupfel::CollisionSystem::CollisionSystem(Registry& reg, EventSystem& evt_sys) : registry(reg), evt_system(evt_sys), worldId({0,0}) {}

void CollisionSystem::Init()
{
	/* Create World */
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = static_cast<b2Vec2>(0.0f, -10.0f);
	worldId = b2CreateWorld(&worldDef);


	b2BodyDef groundBodyDef = b2DefaultBodyDef();
	groundBodyDef.position = static_cast<b2Vec2>(0.0f, -10.0f);
	b2BodyId groundID = b2CreateBody(worldId, &groundBodyDef);

	b2Polygon groundBox = b2MakeBox(50.0f, 10.0f);

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2CreatePolygonShape(groundID, &shapeDef, &groundBox);

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = static_cast<b2Vec2>(0.0f, 4.0f);
	b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
	b2Polygon dynamicBox = b2MakeBox(1.0f, 1.0f);

	shapeDef = b2DefaultShapeDef();
	shapeDef.density = 1.0f;
	shapeDef.material.friction = 0.3f;
	b2CreatePolygonShape(bodyId, &shapeDef, &dynamicBox);
}

void CollisionSystem::Step(double delta, uint32_t sub_steps) {}

void Mupfel::CollisionSystem::SyncTransforms() {}

void Mupfel::CollisionSystem::DispatchEvents() {}

void Mupfel::CollisionSystem::DeInit() { b2DestroyWorld(worldId); }
