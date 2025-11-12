#pragma once
#include "ComponentArray.h"
#include "GPU/GPUVector.h"
#include <cstdint>
#include <cassert>
#include <concepts>
#include <type_traits>

namespace Mupfel {

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	class GPUComponentArray : public ComponentArray<T>
	{
	public:
		using sparse_type = uint32_t;
		using dense_type = uint32_t;
		using component_type = T;

		GPUComponentArray(uint32_t capacity = 1000);
		~GPUComponentArray() override;
		virtual void Remove(Entity e) override;
		virtual bool Has(Entity e) const override;
		T& Get(Entity e) override;
		void Set(Entity e, T val) override;
		void Insert(Entity e, T comp) override;
		virtual uint32_t Size() override;
		uint32_t GetSparseSSBO() const;
		uint32_t GetDenseSSBO() const;
		uint32_t GetComponentSSBO() const;

	private:
		static constexpr uint32_t invalid_entry_32bit = std::numeric_limits<uint32_t>::max();
		GPUVector<uint32_t> sparse;
		GPUVector<uint32_t> dense;
		GPUVector<T> component;
		uint64_t dense_size;
	};

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	inline GPUComponentArray<T>::GPUComponentArray(uint32_t capacity) : dense_size(0)
	{
		uint32_t aligned_capacity = (capacity + 15) & ~15;

		sparse.resize(aligned_capacity, invalid_entry_32bit);
		dense.resize(aligned_capacity, 0);
		component.resize(aligned_capacity, T());
	}

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	inline GPUComponentArray<T>::~GPUComponentArray()
	{
	}

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	inline void GPUComponentArray<T>::Remove(Entity e)
	{
		if (!Has(e))
		{
			return;
		}

		size_t comp_index = sparse[e.Index()];
		size_t last_index = dense_size - 1;

		if (comp_index != last_index)
		{
			/* Swap the element that should be removed with the last one */
			uint32_t tmp_dense = dense[comp_index];
			dense[comp_index] = dense[last_index];
			dense[last_index] = tmp_dense;

			component_type tmp_comp = component[comp_index];
			component[comp_index] = component[last_index];
			component[last_index] = tmp_comp;


			/* the component order changed, update the sparse list */
			sparse[dense[comp_index]] = comp_index;
		}

		if (dense_size > 0)
		{
			dense_size--;
		}

		/* invalidate the component reference */
		sparse[e.Index()] =  invalid_entry_32bit;
	}

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	inline bool GPUComponentArray<T>::Has(Entity e) const
	{
		return e.Index() < sparse.size() && sparse[e.Index()] != invalid_entry_32bit;
	}

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	inline T& GPUComponentArray<T>::Get(Entity e)
	{
		assert(e.Index() < sparse.size());
		assert(sparse[e.Index()] != invalid_entry_32bit && "Entity does not have a component!");

		uint32_t dense_index = sparse[e.Index()];

		return component[dense_index];
	}

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	inline void GPUComponentArray<T>::Set(Entity e, T val)
	{
		assert(e.Index() < sparse.size());
		assert(sparse[e.Index()] != invalid_entry_32bit && "Entity does not have a component!");

		uint32_t dense_index = sparse[e.Index()];
		component[dense_index] = val;
	}

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	inline void GPUComponentArray<T>::Insert(Entity e, T comp)
	{
		/*
			If the sparse array is too small, we resize it using the invalid_index as value.
			We resize in powers of two for now.
		*/
		if (e.Index() >= sparse.size())
		{
			sparse.resize((e.Index() * 2), invalid_entry_32bit);
		}

		/*
			A value of "invalid_index" shows that the entity does not have the component yet.
			Multiple components of the same type for one entity are illegal.
		*/
		assert(sparse[e.Index()] == invalid_entry_32bit && "Entity already has a component of this type!");

		if (dense_size >= dense.size())
		{
			dense.resize((dense_size * 2), 0);
			component.resize((dense_size * 2), T());
		}

		/* Set the dense index in the sparse array */
		sparse[e.Index()] = dense_size;

		/* Set the entity index in the dense array */
		dense[dense_size] = e.Index();

		/* Copy the component */
		component[dense_size] = comp;

		/* Increment the dense index */
		dense_size++;
	}

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	inline uint32_t GPUComponentArray<T>::Size()
	{
		return dense_size;
	}

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	inline uint32_t GPUComponentArray<T>::GetSparseSSBO() const
	{
		return sparse.GetSSBOID();
	}

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	inline uint32_t GPUComponentArray<T>::GetDenseSSBO() const
	{
		return dense.GetSSBOID();
	}

	template<typename T>
		requires std::is_trivially_copyable_v<T>
	inline uint32_t GPUComponentArray<T>::GetComponentSSBO() const
	{
		return component.GetSSBOID();
	}

}



