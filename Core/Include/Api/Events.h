/**
 * \file   Events.h
 * \brief  Easy-to-use functions for interfacing with Mupfel's Event System.
 *
 */
#pragma once
#include "Core/Application.h"
#include "Core/Event.h"
#include "Core/EventSystem.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <utility>

namespace Mupfel::Events
{

/** Post an event of the given type. It can be received on the next frame. */
template <typename T>
	requires EventType<T>
inline void Post(T event)
{
	Application::GetCurrentEventSystem().AddEvent<T>(std::move(event));
}

/** Post an event of the given type. The default constructor of the type will be used. */
template <typename T>
	requires EventType<T>
inline void Post()
{
	Application::GetCurrentEventSystem().AddEvent<T>({});
}

/**
 * Additionally to posting the event (see Post()), invoke all listeners that registered for that event type.
 */
template <typename T>
	requires EventType<T>
inline void PostNow(T event)
{
	Application::GetCurrentEventSystem().AddImmediateEvent<T>(std::move(event));
}

/** Get all events of the given type that were posted last frame. */
template <typename T>
	requires EventType<T>
[[nodiscard]] inline std::span<const T> Get() noexcept
{
	return Application::GetCurrentEventSystem().GetEvents<T>();
}

/** Retrieve the most recent event of the given type, from the last frame. */
template <typename T>
	requires EventType<T>
[[nodiscard]] inline std::optional<T> Latest()
{
	return Application::GetCurrentEventSystem().GetLatestEvent<T>();
}

/** Retrieve the number of events of the given type that are pending. */
template <typename T>
	requires EventType<T>
[[nodiscard]] inline uint64_t Pending()
{
	return Application::GetCurrentEventSystem().GetPendingEvents<T>();
}

/**
 * Register a callback to be invoked when an event of the given type is posted via PostNow().
 *
 */
template <typename T>
	requires EventType<T>
inline void Listen(std::move_only_function<void(const T&)> callback)
{
	Application::GetCurrentEventSystem().RegisterListener<T>(std::move(callback));
}

} // namespace Mupfel::Events
