#include "Registry.h"
#include <cassert>


#include "raylib.h"


using namespace Mupfel;

Entity Registry::CreateEntity() {

	Entity e = entity_manager.CreateEntity();

	/* Update the Component Signature of the Entity */
	if (signatures.size() <= e.Index()) [[unlikely]]
	{
		signatures.resize((signatures.size() + 1) * 2, Entity::Signature(0x0));
	} 
	else {
		signatures[e.Index()] = 0x0;
	}

	/* Update the Archetype Signature of the Entity */
	if (entityToArchetype.size() <= e.Index()) [[unlikely]]
	{
		entityToArchetype.resize((entityToArchetype.size() + 1) * 2, Archetype::Signature(0x0));
	}
	else {
		entityToArchetype[e.Index()] = 0x0;
	}

	/* Entity is created successfully, notify everyone */
	evt_system.AddImmediateEvent<EntityCreatedEvent>(e);

	return e;

}

void Registry::DestroyEntity(Entity e) {
	/* Create an Entity Destroyed Event to give all Listeners time to react */
	evt_system.AddImmediateEvent<EntityDestroyedEvent>(e);

	/* We have to remove the entity from all component lists */
	for (auto&  storage : component_buffer)
	{
		if (storage)
		{
			storage->Remove(e);
		}
		
	}

	entity_manager.DestroyEntity(e);
	signatures[e.Index()].reset();
	entityToArchetype[e.Index()].reset();
}

uint32_t Registry::GetCurrentEntities() const {
	return entity_manager.GetCurrentEntities();
}

Entity::Signature Registry::GetSignature(uint32_t index) const
{
	assert((index < signatures.size()) && "Given Entity was not created correctly!");

	return signatures[index];
}

void Mupfel::Registry::UpdateEntityArchetypes(Entity e)
{
	const auto& entitySig = signatures[e.Index()];

	for (size_t i = 0; i < archetypes.size(); ++i)
	{
		const auto& arch = archetypes[i];

		bool fulfills = (entitySig & arch.comp_signature) == arch.comp_signature;
		bool currentlyIn = entityToArchetype[e.Index()].test(i);

		if (fulfills && !currentlyIn)
		{
			entityToArchetype[e.Index()].set(i);
			archetypeToEntities[i].push_back(e);

			//evt_system.AddImmediateEvent<ArchetypeCompletedEvent>({ e, i });
			TraceLog(LOG_INFO, "Archetype completed");
		}
		else if (!fulfills && currentlyIn)
		{
			entityToArchetype[e.Index()].reset(i);

			auto& vec = archetypeToEntities[i];
			vec.erase(std::remove(vec.begin(), vec.end(), e), vec.end());

			//evt_system.AddImmediateEvent<ArchetypeBrokenEvent>({ e, i });
			TraceLog(LOG_INFO, "Archetype broken");
		}
	}
}