#include "InputManager.h"

using namespace Mupfel;

InputManager::InputManager(EventSystem& evt_system, Mode in_mode) : event_system(evt_system), current_mode(in_mode)
{
	/* First, reset all mappings */
	keyboard_map.fill({UserInput::NONE});
	mouse_map.fill({UserInput::NONE});
	gamepad_map.fill({UserInput::NONE});

	/* Set the GLFW key callback */

	/*
		For now, just manually set the default mappings.
		In the future, we will have some kind of loader that loads
		saved mappings from somewhere (probably a local json file?).
	*/

	MapKeyboardButton(Key::KEY_W, UserInput::MOVE_FORWARD);
	MapKeyboardButton(Key::KEY_A, UserInput::MOVE_LEFT);
	MapKeyboardButton(Key::KEY_S, UserInput::MOVE_BACKKWARDS);
	MapKeyboardButton(Key::KEY_D, UserInput::MOVE_RIGHT);
	MapKeyboardButton(Key::KEY_F, UserInput::WINDOW_FULLSCREEN);
	MapKeyboardButton(Key::KEY_F1, UserInput::TOGGLE_DEBUG_MODE);

	MapMouseButton(MOUSE_BUTTON_LEFT, UserInput::LEFT_MOUSE_CLICK);
	MapMouseButton(MOUSE_BUTTON_RIGHT, UserInput::RIGHT_MOUSE_CLICK);
	MapMouseButton(MOUSE_BUTTON_MIDDLE, UserInput::MIDDLE_MOUSE_CLICK);
}

double Mupfel::InputManager::GetCurrentCursorX() const { return current_mouse_pos_x; }

double Mupfel::InputManager::GetCurrentCursorY() const { return current_mouse_pos_y; }

void Mupfel::InputManager::MapKeyboardButton(Key key, UserInput new_input)
{
	auto key_index = static_cast<size_t>(key);
	if (key_index >= keyboard_map.size())
	{
		return;
	}
	keyboard_map[key_index] = new_input;
}

void Mupfel::InputManager::MapMouseButton(MouseButton button, UserInput new_input)
{
	if (button >= mouse_map.size())
	{
		return;
	}
	mouse_map[button] = new_input;
}

void Mupfel::InputManager::MapGamepadButton(GamepadButton button, UserInput new_input)
{
	if (button >= gamepad_map.size())
	{
		return;
	}
	gamepad_map[button] = new_input;
}

#include <iostream>
void Mupfel::InputManager::UpdateCursor(double new_pos_x, double new_pos_y)
{
	current_mouse_pos_x = new_pos_x;
	current_mouse_pos_y = new_pos_y;
	event_system.AddEvent<UserInputEvent>({UserInput::CURSOR_POS_CHANGED, KeyAction::NONE});
}

void Mupfel::InputManager::UpdateMouseButtons()
{
	UpdateMouseButton(MOUSE_BUTTON_LEFT);
	UpdateMouseButton(MOUSE_BUTTON_RIGHT);
	UpdateMouseButton(MOUSE_BUTTON_MIDDLE);
	UpdateMouseButton(MOUSE_BUTTON_SIDE);
	UpdateMouseButton(MOUSE_BUTTON_EXTRA);
	UpdateMouseButton(MOUSE_BUTTON_FORWARD);
	UpdateMouseButton(MOUSE_BUTTON_BACK);
}

void Mupfel::InputManager::UpdateMouseButton(MouseButton b)
{
#if 0
	/* After that the Mouse presses */
	if (IsMouseButtonPressed(b)) //|| IsMouseButtonDown(b))
	{
		if (mouse_map.at(b).input != UserInput::NONE)
		{
			event_system.AddEvent<UserInputEvent>({ mouse_map.at(b).input });
		}
	}
#endif
}

UserInputEvent::UserInputEvent() : input(UserInput::NONE), action(KeyAction::PRESSED) {}

Mupfel::UserInputEvent::UserInputEvent(UserInput in_input, KeyAction in_action) : input(in_input), action(in_action) {}

void Mupfel::InputManager::KeyPressed(Key key, KeyAction action)
{
	if (current_mode != Mode::MOUSE_KEYBOARD)
	{
		return;
	}

	auto key_index = static_cast<uint32_t>(key);

	/* Check if the key is known. */
	if (key_index >= keyboard_map.size())
	{
		return;
	}

	/* Check if the key is mapped to a function. */
	if (keyboard_map[key_index] != UserInput::NONE)
	{
		event_system.AddEvent<UserInputEvent>({keyboard_map.at(key_index), action});
	}
	
}
