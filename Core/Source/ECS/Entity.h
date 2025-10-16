#pragma once
#include <cstdint>
#include <vector>
#include <bitset>

namespace Mupfel {

	class Entity
	{
	public:
		using Signature = std::bitset<128>;
	public:
		Entity(uint32_t in_index) : index(in_index) {}
		uint32_t Index() const { return index; }
		bool operator==(const Entity& other) const { return index == other.index; }

	private:
		uint32_t index;
	};

	static_assert((sizeof(Entity) == 4) && "Error: Size of Entity Class is not 8 bytes!");


	class EntityManager {
	public:
		EntityManager() : freeList(), current_entities(0), next_entity_index(0) { freeList.reserve(4096); }
		Entity CreateEntity();
		void DestroyEntity(Entity e);
		uint32_t GetCurrentEntities() const;
	private:
		uint32_t current_entities;
		uint32_t next_entity_index;
		std::vector<uint32_t> freeList;
	};

}