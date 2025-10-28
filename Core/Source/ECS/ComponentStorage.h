#pragma once

namespace Mupfel {

	enum class StorageType {
		CPU,
		GPU
	};

	template<typename T>
	class ComponentStorage
	{
	public:
		using const_reference = const T&;
		using reference = T&;
		virtual size_t size() const = 0;
		virtual void push_back(const_reference item) = 0;
		virtual void push_back(T&& item) = 0;
		virtual void pop_back() = 0;
		virtual const_reference Read(size_t index) const = 0;
		virtual reference Read(size_t index) = 0;
		virtual void Write(size_t pos, const_reference val) = 0;
		virtual void clear() = 0;
		virtual uint32_t Id() const = 0;
	};

}