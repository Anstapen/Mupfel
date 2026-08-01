#include "Registry.h"
#include <cassert>

using namespace Mupfel;

Entity Registry::CreateEntity()
{

	Entity e = entity_manager.CreateEntity();

	/* Update the Component Signature of the Entity */
	if (signatures.size() <= e.Index()) [[unlikely]]
	{
		signatures.resize((signatures.size() + 1) * 2, Entity::Signature(0x0));
	}
	else
	{
		signatures[e.Index()] = 0x0;
	}

	/* Update the scene mask of the Entity */
	if (sceneMask.size() <= e.Index()) [[unlikely]]
	{
		sceneMask.resize((sceneMask.size() + 1) * 2, Scene::SceneMask(0x0));
	}

	/*
	 * Must happen after the resize, not as its else-branch: an entity that triggers the grow needs
	 * its mask set too, and a zero mask means "in no scene at all" (unlike a zero signature).
	 */
	sceneMask[e.Index()] = SceneMask(active_scene);

	/* Entity is created successfully, notify everyone */
	evt_system.AddImmediateEvent<EntityCreatedEvent>(e);

	return e;
}

void Registry::DestroyEntity(Entity e)
{
	/* Check if the entity is alive. */
	if (!entity_manager.IsAlive(e))
	{
		return;
	}

	/* Create an Entity Destroyed Event to give all Listeners time to react */
	evt_system.AddImmediateEvent<EntityDestroyedEvent>(e);

	/* We have to remove the entity from all component lists */
	for (uint32_t i = 0; i < component_buffer.size(); i++)
	{
		IComponentArray* storage = component_buffer[i].get();

		if (!storage || !storage->Has(e))
		{
			continue;
		}

		evt_system.AddEvent<ComponentRemovedEvent>({e, signatures[e.Index()], storage->ComponentID()});
		storage->Remove(e);
	}

	entity_manager.DestroyEntity(e);
	signatures[e.Index()].reset();
	sceneMask[e.Index()].reset();
}

uint32_t Registry::GetCurrentEntities() const { return entity_manager.GetCurrentEntities(); }

Entity::Signature Registry::GetSignature(Entity entity) const
{
	assert((entity.Index() < signatures.size()) && "Given Entity was not created correctly!");

	return signatures[entity.Index()];
}

Scene::SceneMask Mupfel::Registry::GetSceneMask(Entity entity) const
{
	assert((entity.Index() < sceneMask.size()) && "Given Entity was not created correctly!");

	return sceneMask[entity.Index()];
}

void Mupfel::Registry::SetActiveScene(SceneHandle scene)
{
	assert((scene < Scene::MAX_SCENES) && "Scene handle out of range!");

	active_scene = scene;
}

SceneHandle Mupfel::Registry::GetActiveScene() const { return active_scene; }

Scene::SceneMask Mupfel::Registry::GetActiveSceneMask() const { return SceneMask(active_scene); }
