#pragma once
#include "Event.h"
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
#include <concepts>

namespace Mupfel
{

/**
 * @brief This interface adds a behavior to that every EventBuffer
 * should implement.
 */
class IEventBuffer
{
public:
	virtual ~IEventBuffer() = default;
	/**
	 * @brief This pure virtual function needs to be implemented to
	 * add a way to retrieve the currently pending events of the
	 * EventBuffer.
	 * @return The currently pending events of the buffer.
	 */
	virtual uint64_t GetPendingEvents() = 0;

	/**
	 * @brief This pure virtual function needs to be implemented to
	 * add a way to clear the EventBuffer.
	 */
	virtual void Clear() = 0;
};

/**
 * @brief The EventBuffer class that actually be instantiated.
 * It basically just wraps a vector of the given type T.
 * @tparam T The Event type that the buffer should hold.
 */
template <typename T>
	requires EventType<T>
class EventBuffer : public IEventBuffer
{
public:
	using const_iterator = typename std::vector<T>::const_iterator;

public:
	/**
	 * @brief The constructor.
	 * @param initial_size The initial size of the underlying vector.
	 */
	EventBuffer(uint32_t initial_size);

	/**
	 * @brief Add an event at the end of the buffer.
	 * @param event The event to be added.
	 */
	void Add(T&& event);

	/**
	 * @brief Get the last event in the buffer.
	 * @return If the buffer currently holds events, retrieve the last one,
	 * std::nunllopt otherwise.
	 */
	std::optional<T> GetLatest();

	/**
	 * @brief Retrieve the begin iterator of the underlying vector.
	 * @return begin iterator.
	 */
	const_iterator begin() const { return event_buf.begin(); }

	/**
	 * @brief Retrieve the end iterator of the underlying vector.
	 * @return end iterator.
	 */
	const_iterator end() const { return event_buf.end(); }

	/**
	 * @brief Clear the buffer.
	 */
	void Clear() override;

	/**
	 * @brief Get amount of events currently in the buffer.
	 * @return Essentially the size of the underlying vector.
	 */
	uint64_t GetPendingEvents() override;

	/**
	 * Returns a span over the vector.
	 * 
	 * \return A span over the underlying vector.
	 */
	std::span<const T> View() const noexcept { return event_buf; }

private:
	/**
	 * @brief The vector that holds the events.
	 */
	std::vector<T> event_buf;
};

template <typename T>
	requires EventType<T>
inline EventBuffer<T>::EventBuffer(uint32_t initial_size) : event_buf()
{
	event_buf.reserve(initial_size);
}

template <typename T>
	requires EventType<T>
inline void EventBuffer<T>::Add(T&& event)
{
	event_buf.emplace_back(std::move(event));
}

template <typename T>
	requires EventType<T>
inline std::optional<T> EventBuffer<T>::GetLatest()
{
	if (!event_buf.empty())
	{
		return event_buf.back();
	}
	else
	{
		return std::nullopt;
	}
}

template <typename T>
	requires EventType<T>
inline void EventBuffer<T>::Clear()
{
	event_buf.clear();
}

template <typename T>
	requires EventType<T>
inline uint64_t EventBuffer<T>::GetPendingEvents()
{
	return event_buf.size();
}
} // namespace Mupfel