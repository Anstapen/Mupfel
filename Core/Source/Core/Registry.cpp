#include "Registry.h"


using namespace Mupfel;

Entity Registry::CreateEntity() {
	return entity_manager.CreateEntity();
}

void Registry::DestroyEntity(Entity e) {
	/* We have to remove the entity from all component lists */
	for (auto& [type, storage] : component_map)
	{
		storage->Remove(e);
	}

	entity_manager.DestroyEntity(e);
}