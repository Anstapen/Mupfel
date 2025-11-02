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
		void push_back(T item) override;
		void pop_back() override;
		const T& Read(size_t index) const override;
		T& Read(size_t index) override;
		void Write(size_t pos, T val) override;
		void clear() override;
		uint32_t Id() const override;
		void resize(size_t new_size) override;
		void swap(size_t first, size_t second) override;
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
	inline void GPUComponentStorage<T>::push_back(T item)
	{
		if ((m_size * sizeof(T)) >= h.capacity)
		{
			uint32_t new_size = (m_size * sizeof(T)) * 2;
			realloc(new_size);
			h.capacity = new_size;
		}

		Write(m_size, item);
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
	inline void GPUComponentStorage<T>::Write(size_t pos, T val)
	{
		mem[pos] = val;
		//GPUAllocator::MemBarrier();
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

	template<typename T>
	inline void GPUComponentStorage<T>::resize(size_t new_size)
	{
		if (new_size < m_size)
		{
			return;
		}

		realloc((new_size + 1) * sizeof(T));

		m_size = new_size;
	}

	template<typename T>
	inline void GPUComponentStorage<T>::swap(size_t first, size_t second)
	{
		T tmp = mem[first];
		mem[first] = mem[second];
		mem[second] = tmp;
	}

}


