#pragma once
#include "Entity.h"
#include "GPU/GPUComponentStorage.h"
#include <vector>
#include <memory>
#include <limits>
#include <cassert>

#include "ECS/Components/ComponentIndex.h"

namespace Mupfel {

	class IArchetypeArray {
	public:
		virtual ~IArchetypeArray() = default;
	};

	class ArchetypeArray : public IArchetypeArray
	{
	public:
		template<typename... Components>
		ArchetypeArray();
		~ArchetypeArray() = default;
	private:
		bool Has(Entity e) const;

		template<typename T>
		T Get(Entity e);

		template<typename T>
		void Set(Entity e, T val);

		template<typename T>
		void Insert(Entity e, T component);

		template<typename T>
		GPUComponentStorage<T>& GetStorage();

		void Remove(Entity e);
		static constexpr size_t invalid_entry = std::numeric_limits<size_t>::max();

		template<typename T>
		void AddComponentBuffer();

		std::vector<size_t> sparse;
		std::vector<uint32_t> dense;
		std::vector<std::unique_ptr<IComponentStorage>> components;
	};

	template<typename ...Components>
	inline ArchetypeArray::ArchetypeArray()
	{
		(AddComponentBuffer<Components>(), ...);
	}

	template<typename T>
	inline void ArchetypeArray::AddComponentBuffer()
	{
		size_t index = ComponentIndex::Index<T>();

		if (index >= components.size())
		{
			components.resize(index + 1);
		}

		components[index] = std::make_unique<GPUComponentStorage<T>>();
	}

	template<typename T>
	inline T ArchetypeArray::Get(Entity e)
	{
		assert(Has(e) && "Given Entity is not an archetype!");
		GPUComponentStorage<T>& storage = GetStorage<T>();

		return storage.Read(sparse[e.Index()]);
	}

	template<typename T>
	inline void ArchetypeArray::Set(Entity e, T val)
	{
		assert(Has(e) && "Given Entity is not an archetype!");
		GPUComponentStorage<T>& storage = GetStorage<T>();

		storage.Write(sparse[e.Index()], val);
	}

	template<typename T>
	inline void ArchetypeArray::Insert(Entity e, T component)
	{
	
		if (e.Index() >= sparse.size())
		{
			sparse.resize((sparse.size() + 2) * 2, invalid_entry);
		}
		
		assert(sparse[e.Index()] == invalid_entry);

		size_t index = dense.size();
		
		sparse[e.Index()] = index;
		
		dense.push_back(e.Index());

		for (auto &c : components)
		{
			if (c)
			{
				c.get()->resize(index + 1);
			}
		}
		
		Set<T>(e, component);

	}

	template<typename T>
	inline GPUComponentStorage<T>& ArchetypeArray::GetStorage()
	{
		size_t index = ComponentIndex::Index<T>();
		assert(index < components.size());
		assert(components[index]);

		return static_cast<GPUComponentStorage<T>*>(components[index].get());
	}
}