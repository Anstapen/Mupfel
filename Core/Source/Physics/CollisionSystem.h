#pragma once
#include "Core/EventSystem.h"
#include "Core/Scene.h"
#include "ECS/Registry.h"
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
public:
	CollisionSystem(Registry& reg, EventSystem& evt_sys);
	void Init();
	void Step(double delta, uint32_t sub_steps);
	void SceneSwitched(SceneHandle new_scene);
	void SyncTransforms();
	void DispatchEvents();
	void DeInit();

private:
	bool	 HasBody(Entity e) const;
	void	 HandlePendingEvents();
	void	 CreateBody(Entity e);
	b2BodyId TakeBody(Entity e);
	void	 SetBody(Entity e, b2BodyId body);
	Entity	 EntityOf(b2ShapeId id);
	b2WorldId WorldForScene(SceneHandle scene);

private:
	Registry&								   registry;
	EventSystem&							   evt_system;
	b2WorldId								   current_world = b2_nullWorldId;
	std::queue<Entity>						   pending_create;
	std::queue<b2BodyId>					   pending_destroy;
	std::mutex								   pending_mutex;
	std::unordered_map<SceneHandle, b2WorldId> worlds;
	std::vector<b2BodyId>					   bodies;
};
} // namespace Mupfel
