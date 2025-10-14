#include "Registry.h"
#include <cassert>


using namespace Mupfel;

Entity Registry::CreateEntity() {

	Entity e = entity_manager.CreateEntity();

	/* Update the Signature of the Entity */
	if (signatures.size() <= e.Index()) [[unlikely]]
	{
		signatures.resize((signatures.size() + 1) * 2, Entity::Signature(0x0));
	} 
	else {
		signatures[e.Index()] = 0x0;
	}

	return e;

}

void Registry::DestroyEntity(Entity e) {
	/* We have to remove the entity from all component lists */
	for (auto& [type, storage] : component_map)
	{
		storage->Remove(e);
	}

	entity_manager.DestroyEntity(e);
	signatures[e.Index()].reset();
}

Entity::Signature Registry::GetSignature(uint32_t index) const
{
	assert((index < signatures.size()) && "Given Entity was not created correctly!");

	return signatures[index];
}
