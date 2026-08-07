/**
 * \file   Input.h
 * \brief  Various input handling utilities.
 *
 */
#pragma once
#include "Core/Application.h"
#include "Core/Event.h"
#include "Core/Input.h"
#include "Core/InputManager.h"
#include <utility>

namespace Mupfel::Input
{

/** The current state of \a key this frame. */
[[nodiscard]] inline KeyAction GetKey(Key key) { return Application::GetKey(key); }

/** The current state of mouse button \a button this frame. */
[[nodiscard]] inline KeyAction GetMouseButton(MouseButton button) { return Application::GetMouseButton(button); }

[[nodiscard]] inline bool CheckUserInput(UserInput ui)
{
	return Application::GetCurrentInputManager().CheckUserInput(ui);
}

/** The cursor's X position in screen space. */
[[nodiscard]] inline double CursorX() { return Application::GetCurrentInputManager().GetCurrentCursorX(); }

/** The cursor's Y position in screen space. */
[[nodiscard]] inline double CursorY() { return Application::GetCurrentInputManager().GetCurrentCursorY(); }

/**
 * Binds \a key so that a copy of \a prototype is posted whenever the key sees one of \a actions.
 *
 * Replaces any binding already installed on that key.
 *
 * \param key       The keyboard key to bind.
 * \param actions   Mask of the actions that should emit (e.g. KeyAction::PRESSED).
 * \param prototype The event object to copy and post.
 */
template <typename T>
	requires EventType<T>
inline void MapKey(Key key, KeyAction actions, T prototype)
{
	Application::GetCurrentInputManager().MapKeyboardButton<T>(key, actions, std::move(prototype));
}

/** Binds a mouse button. See MapKey(). */
template <typename T>
	requires EventType<T>
inline void MapMouse(MouseButton button, KeyAction actions, T prototype)
{
	Application::GetCurrentInputManager().MapMouseButton<T>(button, actions, std::move(prototype));
}

/** Binds a gamepad button. See MapKey(). */
template <typename T>
	requires EventType<T>
inline void MapGamepad(GamepadButton button, KeyAction actions, T prototype)
{
	Application::GetCurrentInputManager().MapGamepadButton<T>(button, actions, std::move(prototype));
}

} // namespace Mupfel::Input
