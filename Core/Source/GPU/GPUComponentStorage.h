#pragma once
#include "ECS/ComponentStorage.h"
#include "GPUAllocator.h"

namespace Mupfel {

	template <typename T>
	class GPUComponentStorage : public ComponentStorage<T>
	{
	public:
		GPUComponentStorage(uint32_t capacity = 10000);
		virtual ~GPUComponentStorage();
		size_t size() const override;
		void push_back(const T& item) override;
		void push_back(T&& item) override;
		void pop_back() override;
		const T& Read(size_t index) const override;
		T& Read(size_t index) override;
		void Write(size_t pos, const T& val) override;
		void clear() override;
		uint32_t Id() const override;
	private:
		void realloc(uint32_t new_size);
	private:
		GPUAllocator::Handle h;
		uint32_t stride = sizeof(T);
		T* mem;
		size_t m_size = 0;
	};

	template<typename T>
	inline void GPUComponentStorage<T>::realloc(uint32_t new_size)
	{
		GPUAllocator::reallocateGPUBuffer(h, new_size);
		mem = static_cast<T*>(h.mapped_ptr);
	}

	template<typename T>
	inline GPUComponentStorage<T>::GPUComponentStorage(uint32_t capacity)
	{
		h = GPUAllocator::allocateGPUBuffer(sizeof(T) * capacity);
		mem = static_cast<T*>(h.mapped_ptr);
	}

	template<typename T>
	inline GPUComponentStorage<T>::~GPUComponentStorage()
	{
		GPUAllocator::freeGPUBuffer(h);
	}

	template<typename T>
	inline size_t GPUComponentStorage<T>::size() const
	{
		return m_size;
	}

	template<typename T>
	inline void GPUComponentStorage<T>::push_back(const T& item)
	{
		if (m_size >= h.capacity)
		{
			uint32_t new_size = (m_size + 1) * 2;
			realloc(new_size);
			h.capacity = new_size;
		}

		mem[m_size] = item;
		m_size++;
	}

	template<typename T>
	inline void GPUComponentStorage<T>::push_back(T&& item)
	{
		if (m_size >= h.capacity)
		{
			uint32_t new_size = (m_size + 1) * 2;
			realloc(new_size);
			h.capacity = new_size;
		}

		mem[m_size] = item;
		m_size++;
	}

	template<typename T>
	inline void GPUComponentStorage<T>::pop_back()
	{
		if (m_size > 0)
		{
			m_size--;
		}
	}

	template<typename T>
	inline const T& GPUComponentStorage<T>::Read(size_t index) const
	{
		return mem[index];
	}

	template<typename T>
	inline T& GPUComponentStorage<T>::Read(size_t index)
	{
		return mem[index];
	}

	template<typename T>
	inline void GPUComponentStorage<T>::Write(size_t pos, const T& val)
	{
		mem[pos] = val;
		GPUAllocator::MemBarrier();
	}

	template<typename T>
	inline void GPUComponentStorage<T>::clear()
	{
		m_size = 0;
	}

	template<typename T>
	inline uint32_t GPUComponentStorage<T>::Id() const
	{
		return h.id;
	}

}


