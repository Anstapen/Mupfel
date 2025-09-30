#pragma once
#include <cstdint>
#include <concepts>
#include <optional>
#include <vector>
#include "Event.h"

namespace Mupfel {
	class IEventBuffer {
	public: 
		virtual uint64_t GetPendingEvents() = 0;
		virtual void Clear() = 0;
	};

	template<typename T>
		requires std::derived_from<T, IEvent>
	class EventBuffer : public IEventBuffer {
	public:
		EventBuffer(uint32_t initial_size);
		void Add(T event);
		std::optional<const T*> Get(uint32_t index);
		std::optional<const T*> GetLatest();
		void Clear() override;
		uint64_t GetPendingEvents() override;
	private:
		std::vector<T> event_buf;
	};

	template<typename T>
		requires std::derived_from<T, IEvent>
	inline EventBuffer<T>::EventBuffer(uint32_t initial_size) : event_buf(initial_size)
	{
		event_buf.clear();
	}

	template<typename T>
		requires std::derived_from<T, IEvent>
	inline void EventBuffer<T>::Add(T event)
	{
		event_buf.emplace_back(event);
	}

	template<typename T>
		requires std::derived_from<T, IEvent>
	inline std::optional<const T*> EventBuffer<T>::Get(uint32_t index)
	{
		if (index < event_buf.size())
		{
			return &event_buf.at(index);
		}
		else {
			return std::nullopt;
		}
	}

	template<typename T>
		requires std::derived_from<T, IEvent>
	inline std::optional<const T*> EventBuffer<T>::GetLatest()
	{
		if (!event_buf.empty())
		{
			return &event_buf.back();
		}
		else {
			return std::nullopt;
		}
	}

	template<typename T>
		requires std::derived_from<T, IEvent>
	inline void EventBuffer<T>::Clear()
	{
		event_buf.clear();
	}

	template<typename T>
		requires std::derived_from<T, IEvent>
	inline uint64_t EventBuffer<T>::GetPendingEvents()
	{
		return event_buf.size();
	}
}