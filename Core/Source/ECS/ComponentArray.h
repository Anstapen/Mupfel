#pragma once
#include <vector>
#include <limits>
#include <cassert>

#include "Entity.h"

#include <iostream>

#include "Core/GUID.h"

namespace Mupfel {

	class IComponentArray {
	public:
		virtual ~IComponentArray() = default;
		virtual void Remove(const Entity& e) = 0;
		virtual bool Has(const Entity& e) const = 0;
	};

	template <typename T>
	class ComponentArray : public IComponentArray
	{
		template<typename... Components> friend class View;
	public:
		void Insert(const Entity& e, const T& component);
		void Remove(const Entity& e) override;
		bool Has(const Entity& e) const override;
		T& Get(const Entity& e);
	private:
		static constexpr size_t invalid_entry = std::numeric_limits<size_t>::max();

		std::vector<size_t> sparse;
		std::vector<uint32_t> dense;
		std::vector<T> components;
	};


	template<typename T>
	inline void ComponentArray<T>::Insert(const Entity& e, const T& component)
	{
		/*
			If the sparse array is too small, we resize it using the invalid_index as value.
			We resize in powers of two for now.
		*/
		if (e.Index() >= sparse.size())
		{
			sparse.resize((sparse.size() + 1) * 2, invalid_entry);
		}

		/*
			A value of "invalid_index" shows that the entity does not have the component yet.
			Multiple components of the same type for one entity are illegal.
		*/
		assert(sparse[e.Index()] == invalid_entry && "Entity already has a component of this type!");

		/*
			New entries of the component and dense vectors are always pushed at the end.
		*/
		sparse[e.Index()] = dense.size();

		/* The value at the index stores which entity uses the component */
		dense.push_back(e.Index());
		components.push_back(component);
	}

	template<typename T>
	inline void ComponentArray<T>::Remove(const Entity& e)
	{
		if (!Has(e))
		{
			return;
		}

		size_t comp_index = sparse[e.Index()];
		size_t last_index = dense.size() - 1;

		/* Swap the element that should be removed with the last one */
		std::swap(dense[comp_index], dense[last_index]);
		std::swap(components[comp_index], components[last_index]);

		/* the component order changed, update the sparse list */
		sparse[dense[comp_index]] = comp_index;

		/* Delete the last component */
		components.pop_back();
		dense.pop_back();

		/* invalidate the component reference */
		sparse[e.Index()] = invalid_entry;
		
	}

	template<typename T>
	inline bool ComponentArray<T>::Has(const Entity& e) const
	{
		return e.Index() < sparse.size() && sparse[e.Index()] != invalid_entry;
	}

	template<typename T>
	inline T& ComponentArray<T>::Get(const Entity& e)
	{
		assert(Has(e) && "Given Entity does not currently have a component of this type!");

		return components[sparse[e.Index()]];
	}
}

