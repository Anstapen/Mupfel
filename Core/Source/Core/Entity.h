#pragma once
#include <cstdint>
#include <vector>

namespace Mupfel {

	class Entity
	{
	public:
		Entity(uint32_t in_index, uint32_t in_gen) : index(in_index), generation(in_gen) {}
		uint32_t Index() const { return index; }
		uint32_t Generation() const { return generation; }
		bool operator==(const Entity& other) const { return index == other.index && generation == other.generation; }

	private:
		uint32_t index;
		uint32_t generation;
	};

	static_assert((sizeof(Entity) == 8) && "Error: Size of Entity Class is not 8 bytes!");


	class EntityManager {
	public:
		EntityManager() : generations(4096), freeList(4096) {}
		Entity CreateEntity();
		void DestroyEntity(Entity e);
		bool isAlive(Entity e) const;
	private:
		std::vector<uint32_t> generations;
		std::vector<uint32_t> freeList;
	};

}