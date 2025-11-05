#pragma once
#include "ComponentArray.h"
#include "GPU/GPUAllocator.h"
#include <cstdint>
#include <cassert>

namespace Mupfel {

	/**
	 * @brief Layout description of GPUComponentArray<T> GPU buffer.
	 *
	 * The GPUComponentArray is a flattened structure that combines three logical
	 * arrays (Sparse, Dense, and Component) into one continuous GPU buffer.
	 *
	 * Each of these sub-arrays has a fixed element count equal to `capacity`.
	 * `capacity` must be a multiple of 16.
	 * Their sizes in bytes differ depending on the element type:
	 *
	 *   total size in bytes =
	 *       capacity * (sizeof(sparse_type)
	 *                 + sizeof(dense_type)
	 *                 + sizeof(component_type))
	 *
	 * ┌───────────────────────────────────────────────────────────────────────────┐
	 * │                      GPUComponentArray<T> Memory Layout                   │
	 * ├───────────────────────────────────────────────────────────────────────────┤
	 * │ Sparse Section                                                            │
	 * │  offset: 0                                                                │
	 * │  size  : capacity * sizeof(sparse_type) bytes                             │
	 * │  type  : sparse_type (usually uint32_t)                                   │
	 * │  use   : Maps Entity-IDs → Dense indices.                                 │
	 * │          Unused entries are filled with `invalid_entry`                   │
	 * │          (all bits set to 1, e.g. 0xFFFFFFFF).                            │
	 * ├───────────────────────────────────────────────────────────────────────────┤
	 * │ Dense Section                                                             │
	 * │  offset: dense_offset_in_bytes = capacity * sizeof(sparse_type)           │
	 * │  size  : capacity * sizeof(dense_type) bytes                              │
	 * │  type  : dense_type (usually uint32_t)                                    │
	 * │  use   : Maps Dense indices → Entity-IDs.                                 │
	 * │          Initially empty (`dense_size = 0`).                              │
	 * ├───────────────────────────────────────────────────────────────────────────┤
	 * │ Component Section                                                         │
	 * │  offset: component_offset_in_bytes =                                      │
	 * │           dense_offset_in_bytes + capacity * sizeof(dense_type)           │
	 * │  size  : capacity * sizeof(component_type) bytes                          │
	 * │  type  : component_type (template parameter T)                            │
	 * │  use   : Holds actual component data for each active Dense entry.         │
	 * │          Grows the same way as the Dense Array (no need for extra size)   │
	 * └───────────────────────────────────────────────────────────────────────────┘
	 *
	 * Example (assuming capacity = 16, T = glm::vec4):
	 *
	 *   sizeof(sparse_type)    = 4 bytes
	 *   sizeof(dense_type)     = 4 bytes
	 *   sizeof(component_type) = 16 bytes
	 *
	 *   ┌─────────────┬────────────┬────────────────────────────┐
	 *   │ Sparse (64B)│ Dense (64B)│ Component (256B)           │
	 *   └─────────────┴────────────┴────────────────────────────┘
	 *   total size = 384 bytes
	 *
	 *   Offsets:
	 *     sparse_offset     = 0
	 *     dense_offset      = 64
	 *     component_offset  = 128
	 *     array_size        = 384
	 */

	template<typename T>
	class GPUComponentArray : public ComponentArray<T>
	{
	public:
		using sparse_type = uint32_t;
		using dense_type = uint32_t;
		using component_type = T;
		
		GPUComponentArray(uint32_t capacity = 16);
		~GPUComponentArray() override;
		virtual void Remove(Entity e) override;
		virtual bool Has(Entity e) const override;
		T Get(Entity e) override;
		void Set(Entity e, T val) override;
		void Insert(Entity e, T component) override;
	private:
		void realloc(uint32_t new_size);
		void resizeDense(uint32_t new_capacity);
		void resizeSparse(uint32_t new_capacity);
		void checkArray() const;
		uint64_t DenseCapacity() const;
		uint64_t SparseCapacity() const;
		uint32_t GetSparseEntry(uint32_t index) const;
		void SetSparseEntry(uint32_t index, sparse_type value);
		uint32_t GetDenseEntry(uint32_t index) const;
		void SetDenseEntry(uint32_t index, dense_type value);
		const T& GetComponentEntry(uint32_t index) const;
		void SetComponentEntry(uint32_t index, const component_type& comp);
		void PushBackComponent(Entity e, const component_type& comp);
		void PopBackComponent();
	private:
		static_assert(sizeof(T) % sizeof(uint32_t) == 0);
		static constexpr uint32_t size_multiplier = sizeof(component_type) / sizeof(dense_type);
		static constexpr uint8_t invalid_entry_8bit = std::numeric_limits<uint8_t>::max();
		static constexpr uint32_t invalid_entry_32bit = std::numeric_limits<uint32_t>::max();
		GPUAllocator::Handle h;
		uint32_t stride = sizeof(T);
		uint64_t array_size_in_bytes;
		uint64_t dense_offset_in_bytes;
		uint64_t dense_size;
		uint64_t component_offset_in_bytes;
	};

	template<typename T>
	inline GPUComponentArray<T>::GPUComponentArray(uint32_t capacity)
	{
		/* Make sure that capacity is a multiple of 16 */
		uint32_t aligned_capacity = (capacity + 15) & ~15;
		uint64_t initial_cap_in_bytes = aligned_capacity * sizeof(sparse_type) + aligned_capacity * sizeof(dense_type) + aligned_capacity * sizeof(component_type);
		array_size_in_bytes = initial_cap_in_bytes;
		dense_offset_in_bytes = aligned_capacity * sizeof(sparse_type);
		dense_size = 0;
		component_offset_in_bytes = dense_offset_in_bytes + aligned_capacity * sizeof(dense_type);
		h = GPUAllocator::allocateGPUBuffer(initial_cap_in_bytes);

		/* Set the sparse part of the array to invalid_entry */
		std::memset(h.mapped_ptr, invalid_entry_8bit, aligned_capacity * sizeof(sparse_type));
	}

	template<typename T>
	inline GPUComponentArray<T>::~GPUComponentArray()
	{
		GPUAllocator::freeGPUBuffer(h);
	}

	template<typename T>
	inline void GPUComponentArray<T>::Remove(Entity e)
	{
		if (!Has(e))
		{
			return;
		}

		size_t comp_index = GetSparseEntry(e.Index());
		size_t last_index = dense_size - 1;

		if (comp_index != last_index)
		{
			/* Swap the element that should be removed with the last one */
			uint32_t tmp_dense = GetDenseEntry(comp_index);
			SetDenseEntry(comp_index, GetDenseEntry(last_index));
			SetDenseEntry(last_index, tmp_dense);

			component_type tmp_comp = GetComponentEntry(comp_index);
			SetComponentEntry(comp_index, GetComponentEntry(last_index));
			SetComponentEntry(last_index, tmp_comp);


			/* the component order changed, update the sparse list */
			SetSparseEntry(GetDenseEntry(comp_index), comp_index);
		}

		/* Delete the last component */
		PopBackComponent();

		/* invalidate the component reference */
		SetSparseEntry(e.Index(), invalid_entry_32bit);
	}

	template<typename T>
	inline bool GPUComponentArray<T>::Has(Entity e) const
	{
		return e.Index() < SparseCapacity() && GetSparseEntry(e.Index()) != invalid_entry_32bit;
	}

	template<typename T>
	inline T GPUComponentArray<T>::Get(Entity e)
	{
		assert(e.Index() < SparseCapacity());
		assert(GetSparseEntry(e.Index()) != invalid_entry_32bit && "Entity does not have a component!");

		uint32_t dense_index = GetSparseEntry(e.Index());

		return GetComponentEntry(dense_index);
	}

	template<typename T>
	inline void GPUComponentArray<T>::Set(Entity e, T val)
	{
		assert(e.Index() < SparseCapacity());
		assert(GetSparseEntry(e.Index()) != invalid_entry_32bit && "Entity does not have a component!");

		uint32_t dense_index = GetSparseEntry(e.Index());
		SetComponentEntry(dense_index, val);
	}

	template<typename T>
	inline void GPUComponentArray<T>::Insert(Entity e, T component)
	{
		/*
			If the sparse array is too small, we resize it using the invalid_index as value.
			We resize in powers of two for now.
		*/
		if (e.Index() >= SparseCapacity())
		{
			resizeSparse((e.Index()) * 2);
		}

		/*
			A value of "invalid_index" shows that the entity does not have the component yet.
			Multiple components of the same type for one entity are illegal.
		*/
		uint32_t index = e.Index();
		assert(GetSparseEntry(e.Index()) == invalid_entry_32bit && "Entity already has a component of this type!");

		PushBackComponent(e, component);
	}

	template<typename T>
	inline void GPUComponentArray<T>::realloc(uint32_t new_size)
	{
		GPUAllocator::reallocateGPUBuffer(h, new_size);
	}

	template<typename T>
	inline void GPUComponentArray<T>::resizeDense(uint32_t new_capacity)
	{
		/* Make sure that capacity is a multiple of 16 */
		uint32_t aligned_capacity = (new_capacity + 15) & ~15;

		/* size of the sparse section stays the same, but we double the size of dense and component */
		/* Calculate the new size of the array */
		
		uint64_t new_sparse_cap_in_bytes = SparseCapacity() * sizeof(sparse_type);
		uint64_t new_dense_cap_in_bytes = (aligned_capacity * sizeof(dense_type));
		uint64_t new_comp_cap_in_bytes = (aligned_capacity * sizeof(component_type));
		uint64_t new_array_size_in_bytes = new_sparse_cap_in_bytes + new_dense_cap_in_bytes + new_comp_cap_in_bytes;
		realloc(new_array_size_in_bytes);

		/* copy the existing component data into a temp buffer */
		std::vector<uint8_t> tmp;
		tmp.resize(dense_size * sizeof(component_type));
		auto mapped_ptr_in_bytes = static_cast<uint8_t*>(h.mapped_ptr);
		mapped_ptr_in_bytes += component_offset_in_bytes;
		std::memcpy(tmp.data(), mapped_ptr_in_bytes, dense_size * sizeof(component_type));

		/* Clear added bytes of the dense array */
		auto dense_ptr = static_cast<uint8_t*>(h.mapped_ptr) + dense_offset_in_bytes;
		uint64_t new_dense_bytes = (aligned_capacity - DenseCapacity()) * sizeof(dense_type);
		std::memset(dense_ptr + DenseCapacity() * sizeof(dense_type), 0, new_dense_bytes);

		/* Copy the component data back */
		mapped_ptr_in_bytes = static_cast<uint8_t*>(h.mapped_ptr);
		mapped_ptr_in_bytes += new_sparse_cap_in_bytes + new_dense_cap_in_bytes;
		std::memcpy(mapped_ptr_in_bytes, tmp.data(), tmp.size());

		/* Update the component offset and array size */
		component_offset_in_bytes = dense_offset_in_bytes + (aligned_capacity * sizeof(dense_type));
		array_size_in_bytes = new_array_size_in_bytes;
	}

	template<typename T>
	inline void GPUComponentArray<T>::resizeSparse(uint32_t new_capacity)
	{
		/* Make sure that capacity is a multiple of 16 */
		uint32_t aligned_capacity = (new_capacity + 15) & ~15;

		/* we double the size of the sparse section, dense and components stay the same */
		/* Calculate the new size of the array */

		uint64_t new_sparse_cap_in_bytes = aligned_capacity * sizeof(sparse_type);
		uint64_t old_dense_cap_in_bytes = DenseCapacity() * sizeof(dense_type);
		uint64_t old_comp_cap_in_bytes = (DenseCapacity() * sizeof(dense_type) * size_multiplier);
		uint64_t new_array_size_in_bytes = new_sparse_cap_in_bytes + old_dense_cap_in_bytes + old_comp_cap_in_bytes;
		

		/* copy the dense and component data */
		std::vector<uint8_t> tmp;
		uint64_t size_of_old_dense_and_comp = old_dense_cap_in_bytes + old_comp_cap_in_bytes;
		tmp.resize(size_of_old_dense_and_comp);
		auto mapped_ptr_in_bytes = static_cast<uint8_t*>(h.mapped_ptr);
		mapped_ptr_in_bytes += dense_offset_in_bytes;
		std::memcpy(tmp.data(), mapped_ptr_in_bytes, size_of_old_dense_and_comp);

		/* Reallocate the buffer */
		realloc(new_array_size_in_bytes);

		/* Mark the new bytes of the sparse array as invalid */
		auto sparse_ptr = static_cast<uint8_t*>(h.mapped_ptr);
		uint64_t old_sparse_capacity = SparseCapacity();
		uint64_t old_sparse_bytes = old_sparse_capacity * sizeof(sparse_type);
		uint64_t new_sparse_bytes = (aligned_capacity - old_sparse_capacity) * sizeof(sparse_type);

		printf("SparseCap old=%llu new=%u\n", old_sparse_capacity, aligned_capacity);
		printf("Fill from %llu bytes, len %llu\n", old_sparse_bytes, new_sparse_bytes);
		std::memset(sparse_ptr + old_sparse_bytes, invalid_entry_8bit, new_sparse_bytes);

		auto test_ptr = static_cast<uint32_t*>(h.mapped_ptr);
		/* Check memory */
		for (uint32_t i = (old_sparse_bytes / 4) - 1000; i < (old_sparse_bytes / 4) + 1000; i++)
		{
			if (test_ptr[i] != invalid_entry_32bit)
			{
				//printf("Entry %u: %u\n", i, test_ptr[i]);
			}
		}

		/* Copy the dense and component data back */
		mapped_ptr_in_bytes = static_cast<uint8_t*>(h.mapped_ptr);
		mapped_ptr_in_bytes += new_sparse_cap_in_bytes;
		std::memcpy(mapped_ptr_in_bytes, tmp.data(), tmp.size());

		/* Update the component offset, the dense offset and array size */
		dense_offset_in_bytes = new_sparse_cap_in_bytes;
		component_offset_in_bytes = dense_offset_in_bytes + old_dense_cap_in_bytes;
		array_size_in_bytes = new_array_size_in_bytes;
	}

	template<typename T>
	inline void GPUComponentArray<T>::checkArray() const
	{
		uint64_t sparse_capacity = dense_offset_in_bytes;
		uint64_t dense_capacity = component_offset_in_bytes - dense_offset_in_bytes;
		uint64_t component_capacity = array_size_in_bytes - component_offset_in_bytes;
		
		assert((sparse_capacity + dense_capacity + component_capacity) == array_size_in_bytes);
	}

	template<typename T>
	inline uint64_t GPUComponentArray<T>::DenseCapacity() const
	{
		assert((component_offset_in_bytes - dense_offset_in_bytes) % sizeof(dense_type) == 0);
		return (component_offset_in_bytes - dense_offset_in_bytes) / sizeof(dense_type);
	}

	template<typename T>
	inline uint64_t GPUComponentArray<T>::SparseCapacity() const
	{
		assert(dense_offset_in_bytes % sizeof(sparse_type) == 0);
		return dense_offset_in_bytes / sizeof(sparse_type);
	}

	template<typename T>
	inline uint32_t GPUComponentArray<T>::GetSparseEntry(uint32_t index) const
	{
		assert(index < SparseCapacity());
		sparse_type* sparse_ptr = static_cast<sparse_type*>(h.mapped_ptr);

		uint32_t value = sparse_ptr[index];

		//printf("GetSparseEntry: %u\n", value);

		return sparse_ptr[index];
	}

	template<typename T>
	inline void GPUComponentArray<T>::SetSparseEntry(uint32_t index, sparse_type value)
	{
		assert(index < SparseCapacity());
		sparse_type* sparse_ptr = static_cast<sparse_type*>(h.mapped_ptr);

		sparse_ptr[index] = value;
	}

	template<typename T>
	inline uint32_t GPUComponentArray<T>::GetDenseEntry(uint32_t index) const
	{
		assert(index < dense_size);
		uint8_t* base_ptr = static_cast<uint8_t*>(h.mapped_ptr);
		base_ptr += dense_offset_in_bytes;
		dense_type* dense_ptr = reinterpret_cast<dense_type*>(base_ptr);

		return dense_ptr[index];
	}

	template<typename T>
	inline void GPUComponentArray<T>::SetDenseEntry(uint32_t index, dense_type value)
	{
		assert(index <= dense_size);
		uint8_t* base_ptr = static_cast<uint8_t*>(h.mapped_ptr);
		base_ptr += dense_offset_in_bytes;
		dense_type* dense_ptr = reinterpret_cast<dense_type*>(base_ptr);

		dense_ptr[index] = value;
	}

	template<typename T>
	inline const T& GPUComponentArray<T>::GetComponentEntry(uint32_t index) const
	{
		assert(index < dense_size);
		uint8_t* base_ptr = static_cast<uint8_t*>(h.mapped_ptr);
		base_ptr += component_offset_in_bytes;
		component_type* component_ptr = reinterpret_cast<component_type*>(base_ptr);

		return component_ptr[index];
	}

	template<typename T>
	inline void GPUComponentArray<T>::SetComponentEntry(uint32_t index, const component_type& comp)
	{
		assert(index <= dense_size);
		uint8_t* base_ptr = static_cast<uint8_t*>(h.mapped_ptr);
		base_ptr += component_offset_in_bytes;
		component_type* component_ptr = reinterpret_cast<component_type*>(base_ptr);

		component_ptr[index] = comp;
	}

	template<typename T>
	inline void GPUComponentArray<T>::PushBackComponent(Entity e, const component_type& comp)
	{
		if (dense_size >= DenseCapacity())
		{
			resizeDense((dense_size) * 2);
		}

		assert(e.Index() < SparseCapacity());

		/* Set the dense index in the sparse array */
		SetSparseEntry(e.Index(), dense_size);

		/* Set the entity index in the dense array */
		SetDenseEntry(dense_size, e.Index());

		/* Copy the component */
		SetComponentEntry(dense_size, comp);

		/* Increment the dense index */
		dense_size++;
	}

	template<typename T>
	inline void GPUComponentArray<T>::PopBackComponent()
	{
		if (dense_size > 0)
		{
			dense_size--;
		}
	}

}

