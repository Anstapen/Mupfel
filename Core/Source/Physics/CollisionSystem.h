#pragma once
#include "Core/EventSystem.h"
#include "Core/Scene.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Logger.h"
#include "box2d/box2d.h"
#include <cstdint>
#include <mutex>
#include <queue>
#include <unordered_map>

namespace Mupfel
{

class CollisionSystem
{
	friend class DebugLayer;
	friend class Registry;

public:
	CollisionSystem(Registry& reg, EventSystem& evt_sys);
	void Init();
	void Step(double delta, uint32_t sub_steps);
	void SceneSwitched(SceneHandle new_scene);
	void SyncTransforms();
	void DispatchEvents();
	void DeInit();

	void SetTransform(Entity e, Transform t);
	void SetMovement(Entity e, float vel_x, float vel_y, float vel_ang);

private:
	bool	  HasBody(Entity e) const;
	void	  HandlePendingEvents();
	void	  CreateBody(Entity e);
	void	  CreateCollider(Entity e);
	b2BodyId  TakeBody(Entity e);
	void	  SetBody(Entity e, b2BodyId body);
	Entity	  EntityOf(b2ShapeId id);
	Entity	  EntityOf(b2BodyId id);
	b2WorldId WorldForScene(SceneHandle scene);

private:
	Registry&								   registry;
	EventSystem&							   evt_system;
	Logger::SafeLoggerPtr					   logger;
	b2WorldId								   current_world = b2_nullWorldId;
	std::queue<Entity>						   pending_body_create;
	std::queue<Entity>						   pending_collider_create;
	std::queue<b2BodyId>					   pending_destroy;
	std::mutex								   pending_mutex;
	std::unordered_map<SceneHandle, b2WorldId> worlds;
	std::vector<b2BodyId>					   bodies;
};
} // namespace Mupfel
