#pragma once
#include "Event.h"
#include "EventSystem.h"
#include "Input.h"
#include <array>
#include <cstdint>

struct GLFWwindow;
typedef struct GLFWwindow GLFWwindow;

namespace Mupfel
{

class Window;

/**
 * @brief Event class representing an input action triggered by the user.
 *
 * This event type is generated whenever a mapped key, mouse button,
 * or gamepad button is pressed, or when the cursor position changes.
 */
class UserInputEvent : public Event
{
public:
	/**
	 * @brief Default constructor. Creates a UserInputEvent with no active input.
	 */
	UserInputEvent();

	/**
	 * @brief Constructs a UserInputEvent for the given user input.
	 * @param in_input The type of user input that occurred.
	 */
	UserInputEvent(UserInput in_input, KeyAction in_action);

	/**
	 * @brief Default Destructor.
	 */
	virtual ~UserInputEvent() = default;

public:
	/** @brief The user input action represented by this event. */
	UserInput input;
	KeyAction action;
};

struct Binding
{
	KeyAction						  actionMask = KeyAction::NONE;
	std::function<void(EventSystem&)> emitter = nullptr;
};

/**
 * @brief The InputManager handles user input from the keyboard, mouse, and gamepad.
 *
 * It scans input devices each frame and generates corresponding
 * UserInputEvent instances that are pushed into the EventSystem.
 *
 * Which UserInputEvent is issued is based on a mapping that can be edited at runtime.
 */
class InputManager
{
	friend class Window;

public:
	/**
	 * @brief The Input Manager either listens to Gamepad or Mouse + Keyboard events.
	 */
	enum class Mode
	{
		MOUSE_KEYBOARD,
		GAMEPAD
	};

public:
	/**
	 * @brief Constructs the Input Manager and initializes default mappings.
	 * @param evt_system Reference to the global EventSystem.
	 * @param in_mode The active input mode (default: Mouse + Keyboard).
	 */
	InputManager(EventSystem& evt_system, Mode in_mode = Mode::MOUSE_KEYBOARD);

	/**
	 * @brief Destructor
	 */
	virtual ~InputManager() = default;

	/**
	 * @brief Returns the current X position of the mouse cursor.
	 * @return The X coordinate of the cursor in screen space.
	 */
	double GetCurrentCursorX() const;

	/**
	 * @brief Returns the current Y position of the mouse cursor.
	 * @return The Y coordinate of the cursor in screen space.
	 */
	double GetCurrentCursorY() const;

	bool CheckUserInput(UserInput ui) const;

	/**
	 * Map a Keyboard button to an Event object.
	 *
	 * When the key \a key is pressed, the provided event object is emitted.
	 *
	 * \param key The keyboard key on which the event should be emitted.
	 * \param in_action_mask The key actions on which the event should be emitted.
	 * \param prototype The event object that should be emitted.
	 */
	template <typename T>
		requires std::derived_from<T, Event> && std::copy_constructible<T>
	void MapKeyboardButton(Key key, KeyAction in_action_mask, T prototype);

	/**
	 * Map a mouse button to an Event object.
	 *
	 * When the key \a key is pressed, the provided event object is emitted.
	 *
	 * \param button The mouse button on which the event should be emitted.
	 * \param in_action_mask The key actions on which the event should be emitted.
	 * \param prototype The event object that should be emitted.
	 */
	template <typename T>
		requires std::derived_from<T, Event> && std::copy_constructible<T>
	void MapMouseButton(MouseButton button, KeyAction in_action_mask, T prototype);

	/**
	 * Map a gamepad button to an Event object.
	 *
	 * When the key \a key is pressed, the provided event object is emitted.
	 *
	 * \param button The gamepad button on which the event should be emitted.
	 * \param in_action_mask The key actions on which the event should be emitted.
	 * \param prototype The event object that should be emitted.
	 */
	template <typename T>
		requires std::derived_from<T, Event> && std::copy_constructible<T>
	void MapGamepadButton(GamepadButton button, KeyAction in_action_mask, T prototype);

protected:
	/**
	 * @brief Updates the Cursor (Mouse or Gamepad joystick, depending on the mode).
	 */
	void UpdateCursor(double new_pos_x, double new_pos_y);

	void KeyPressed(Key key, KeyAction action);

	void MouseButtonPressed(MouseButton b, KeyAction action);

private:
	/** @brief The active input mode (Mouse + Keyboard or Gamepad). */
	Mode current_mode;

	/** @brief Reference to the EventSystem used to dispatch input events. */
	EventSystem& event_system;

	/** @brief Mapping of keyboard keys to input events. */
	std::array<Binding, 512> keyboard_map;

	/** @brief Mapping of mouse buttons to input events. */
	std::array<Binding, 16> mouse_map;

	/** @brief Mapping of gamepad buttons to input events. */
	std::array<Binding, 32> gamepad_map;

	double current_mouse_pos_x = 0.0f;
	double current_mouse_pos_y = 0.0f;
};

template <typename T>
	requires std::derived_from<T, Event> && std::copy_constructible<T>
inline void InputManager::MapKeyboardButton(Key key, KeyAction in_action_mask, T prototype)
{
	const auto idx = static_cast<size_t>(key);
	if (idx >= keyboard_map.size())
	{
		return;
	}
	keyboard_map[idx].emitter = [p = std::move(prototype)](EventSystem& es) { es.AddEvent<T>(T(p)); };
	keyboard_map[idx].actionMask = in_action_mask;
}

template <typename T>
	requires std::derived_from<T, Event> && std::copy_constructible<T>
inline void InputManager::MapMouseButton(MouseButton button, KeyAction in_action_mask, T prototype)
{
	const auto idx = static_cast<size_t>(button);
	if (idx >= mouse_map.size())
	{
		return;
	}
	mouse_map[idx].emitter = [p = std::move(prototype)](EventSystem& es) { es.AddEvent<T>(T(p)); };
	mouse_map[idx].actionMask = in_action_mask;
}

template <typename T>
	requires std::derived_from<T, Event> && std::copy_constructible<T>
inline void InputManager::MapGamepadButton(GamepadButton button, KeyAction in_action_mask, T prototype)
{
	const auto idx = static_cast<size_t>(button);
	if (idx >= gamepad_map.size())
	{
		return;
	}
	gamepad_map[idx].emitter = [p = std::move(prototype)](EventSystem& es) { es.AddEvent<T>(T(p)); };
	gamepad_map[idx].actionMask = in_action_mask;
}
} // namespace Mupfel