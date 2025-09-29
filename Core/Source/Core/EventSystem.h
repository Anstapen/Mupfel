#pragma once
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <optional>
#include "GUID.h"
#include "EventBuffer.h"

namespace Mupfel {

	class EventSystem {
	public:
		typedef std::unordered_map<uint32_t, std::unique_ptr<IEventBuffer>> EventBufferMap;
	public:
		EventSystem();

		void Update();

		template<typename T>
			requires std::derived_from<T, IEvent>
		void AddEvent(T event);

		template<typename T>
			requires std::derived_from<T, IEvent>
		uint64_t GetPendingEvents();

		template<typename T>
			requires std::derived_from<T, IEvent>
		std::optional<const T*> GetEvent(uint32_t index);

		template<typename T>
			requires std::derived_from<T, IEvent>
		std::optional<const T*> GetLatestEvent();

	private:
		EventBufferMap event_map[2];
		uint32_t current;
		uint32_t next;
	};

	template<typename T>
		requires std::derived_from<T, IEvent>
	inline void EventSystem::AddEvent(T event)
	{
		constexpr uint64_t evt_guid = event.GetGUID();

		auto [it, inserted] = event_map[next].try_emplace(evt_guid, std::make_unique<EventBuffer<T>>(16));
		
		EventBuffer<T>* current_evt_buffer = static_cast<EventBuffer<T> *>(it->second.get());

		current_evt_buffer->Add(event);
	}

	template<typename T>
		requires std::derived_from<T, IEvent>
	inline std::optional<const T*> EventSystem::GetEvent(uint32_t index)
	{
		T event;
		constexpr uint64_t evt_guid = event.GetGUID();
		auto it = event_map[current].find(evt_guid);

		if (it != event_map[current].end())
		{
			EventBuffer<T> *buf = static_cast<EventBuffer<T> *>(it->second.get());
			return buf->Get(index);
		}
		else {
			return std::nullopt;
		}
	}

	template<typename T>
		requires std::derived_from<T, IEvent>
	inline std::optional<const T*> EventSystem::GetLatestEvent()
	{
		T event;
		auto it = event_map[current].find(event.GetGUID());

		if (it != event_map[current].end())
		{
			EventBuffer<T>* buf = static_cast<EventBuffer<T> *>(it->second.get());
			return buf->GetLatest();
		}
		else {
			return std::nullopt;
		}
	}

	template<typename T>
		requires std::derived_from<T, IEvent>
	inline uint64_t EventSystem::GetPendingEvents()
	{
		T event;
		auto it = event_map[current].find(event.GetGUID());

		if (it != event_map[current].end())
		{
			return it->second.get()->GetPendingEvents();
		}
		else {
			return 0;
		}
	}

}