#pragma once
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <optional>
#include "GUID.h"
#include "EventBuffer.h"

namespace Mupfel {

	/**
	 * @brief The EventSystem class.
	 * 
	 * The EventSystem class is responsible for storing all events that get issued by
	 * the Engine Subsystems and custom systems implemented by the Application that
	 * uses Mupfel.
	 * 
	 * Currently the Event System supports asynchronous events that are handled on a
	 * per-frame basis:
	 * Every Event that gets added is available in the next frame. So all events have
	 * a inherent delay of one frame.
	 * Every Event only exists for one frame, specifically the frame after the one in
	 * which it gets added.
	 * 
	 * Support for synchronous event handling will probably be added in the future.
	 */
	class EventSystem {
	public:
		/**
		 * @brief The Event System holds one EventBuffer for every Event type that gets
		 * added and stores them in an unordered map. To be safe regarding memory leaks,
		 * the Eventbuffers are stored as unique pointers.
		 */
		typedef std::unordered_map<uint32_t, std::unique_ptr<IEventBuffer>> EventBufferMap;
	public:
		/**
		 * @brief The construcor of the EventSystem class.
		 */
		EventSystem();

		/**
		 * @brief Swaps the EventBufferMaps. Should be called at the beginning or the
		 * end of every frame, but to more than one time per frame.
		 */
		void Update();

		/**
		 * @brief Add an Event to its Eventbuffer. If there is no EventBuffer for that
		 * Event type yet, it creates one.
		 * @tparam T The type of Event that should be added.
		 * @param event The event data.
		 */
		template<typename T>
			requires std::derived_from<T, IEvent>
		void AddEvent(T &&event);

		/**
		 * @brief Get the amount of events currently pending for the specified
		 * Event type.
		 * @tparam T The Event type.
		 * @return The number of events pending for the given Event type.
		 */
		template<typename T>
			requires std::derived_from<T, IEvent>
		uint64_t GetPendingEvents();

		/**
		 * @brief Retieve an Event of type \p T at the given index.
		 * @tparam T The type of the wanted event.
		 * @param index The index of the wanted event.
		 * @return The event.
		 */
		template<typename T>
			requires std::derived_from<T, IEvent>
		std::optional<const T*> GetEvent(uint32_t index);

		/**
		 * @brief Retrieve the latest event of the given type.
		 * @tparam T The type of the event.
		 * @return The latest event.
		 */
		template<typename T>
			requires std::derived_from<T, IEvent>
		std::optional<const T*> GetLatestEvent();

	private:
		/**
		 * @brief Two maps that hold the eventbuffers and get swapped every frame.
		 */
		EventBufferMap event_map[2];

		/**
		 * @brief The index of the current EventBufferMap. Events that are read 
		 * this frame are read from this map.
		 */
		uint32_t current;

		/**
		 * @brief The index of the next EventBufferMap. Events that are added
		 * this frame are put in this buffer and are available in the next frame.
		 */
		uint32_t next;
	};

	template<typename T>
		requires std::derived_from<T, IEvent>
	inline void EventSystem::AddEvent(T &&event)
	{
		T local_event;
		constexpr uint64_t evt_guid = local_event.GetGUID();

		auto [it, inserted] = event_map[next].try_emplace(evt_guid, std::make_unique<EventBuffer<T>>(16));
		
		EventBuffer<T>* current_evt_buffer = static_cast<EventBuffer<T> *>(it->second.get());

		current_evt_buffer->Add(std::move(event));
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