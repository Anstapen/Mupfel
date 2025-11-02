#pragma once

namespace Mupfel {

	enum class StorageType {
		CPU,
		GPU
	};

	class IComponentStorage {
	public:
		virtual void resize(size_t new_size) = 0;
		virtual void swap(size_t first, size_t second) = 0;
		virtual void pop_back() = 0;
		virtual void clear() = 0;
		virtual size_t size() const = 0;
		virtual uint32_t Id() const = 0;
	};

	template<typename T>
	class ComponentStorage : public IComponentStorage {
	public:
		using const_reference = const T&;
		using reference = T&;
		virtual void push_back(T item) = 0;
		virtual const_reference Read(size_t index) const = 0;
		virtual reference Read(size_t index) = 0;
		virtual void Write(size_t pos, T val) = 0;
	};

}