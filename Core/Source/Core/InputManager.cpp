#include "InputManager.h"

#include "raylib.h"

using namespace Mupfel;

static Vector2 current_mouse_pos;

InputManager::InputManager(EventSystem& evt_system, Mode in_mode) : event_system(evt_system), current_mode(in_mode)
{
	/* First, reset all mappings */
	keyboard_map.fill({ UserInput::NONE });
	mouse_map.fill({ UserInput::NONE });
	gamepad_map.fill({ UserInput::NONE });

	/* 
		For now, just manually set the default mappings.
		In the future, we will have some kind of loader that loads
		saved mappings from somewhere (probably a local json file?).
	*/

	MapKeyboardButton(KEY_W, UserInput::MOVE_FORWARD);
	MapKeyboardButton(KEY_A, UserInput::MOVE_LEFT);
	MapKeyboardButton(KEY_S, UserInput::MOVE_BACKKWARDS);
	MapKeyboardButton(KEY_D, UserInput::MOVE_RIGHT);
	MapKeyboardButton(KEY_F, UserInput::WINDOW_FULLSCREEN);
	MapKeyboardButton(KEY_F1, UserInput::TOGGLE_DEBUG_MODE);

	MapMouseButton(MOUSE_BUTTON_LEFT, UserInput::LEFT_MOUSE_CLICK);
	MapMouseButton(MOUSE_BUTTON_RIGHT, UserInput::RIGHT_MOUSE_CLICK);
	MapMouseButton(MOUSE_BUTTON_MIDDLE, UserInput::MIDDLE_MOUSE_CLICK);

}

void InputManager::Update(double elapsedTime)
{
	UpdateButtons();
	UpdateCursor();
}

float Mupfel::InputManager::GetCurrentCursorX() const
{
	return current_mouse_pos.x;
}

float Mupfel::InputManager::GetCurrentCursorY() const
{
	return current_mouse_pos.y;
}

void Mupfel::InputManager::MapKeyboardButton(Key key, UserInput new_input)
{
	if (key >= keyboard_map.size())
	{
		return;
	}
	keyboard_map[key] = new_input;
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

void Mupfel::InputManager::UpdateButtons()
{
	/* First update keyboard presses */

	uint32_t key = GetKeyPressed();

	while (key != 0)
	{
		if (keyboard_map.at(key).input != UserInput::NONE)
		{
			event_system.AddEvent<UserInputEvent>({ keyboard_map.at(key).input });
		}

		key = GetKeyPressed();
	}

	UpdateMouseButtons();

	/* Get Gamepad Buttons */

	key = GetGamepadButtonPressed();

	if (gamepad_map.at(key).input != UserInput::NONE)
	{
		event_system.AddEvent<UserInputEvent>({ gamepad_map.at(key).input });
	}

}

void Mupfel::InputManager::UpdateCursor()
{
	Vector2 current_pos = GetMousePosition();

	if (current_pos.x != current_mouse_pos.x || current_pos.y != current_mouse_pos.y)
	{
		current_mouse_pos = current_pos;
		event_system.AddEvent<UserInputEvent>({UserInput::CURSOR_POS_CHANGED});
	}
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
	/* After that the Mouse presses */
	if (IsMouseButtonPressed(b)) //|| IsMouseButtonDown(b))
	{
		if (mouse_map.at(b).input != UserInput::NONE)
		{
			event_system.AddEvent<UserInputEvent>({ mouse_map.at(b).input });
		}
	}
}

UserInputEvent::UserInputEvent() : input(UserInput::NONE)
{
}

Mupfel::UserInputEvent::UserInputEvent(UserInput in_input) : input(in_input)
{
}
