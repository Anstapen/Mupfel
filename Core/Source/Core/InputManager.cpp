#include "InputManager.h"

#include "raylib.h"

using namespace Mupfel;

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

	MapMouseButton(MOUSE_BUTTON_LEFT, UserInput::LEFT_MOUSE_CLICK);
	MapMouseButton(MOUSE_BUTTON_RIGHT, UserInput::RIGHT_MOUSE_CLICK);
	MapMouseButton(MOUSE_BUTTON_MIDDLE, UserInput::MIDDLE_MOUSE_CLICK);

}

void InputManager::Update(float elapsedTime)
{
	UpdateButtons();
	UpdateCursor();
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

	/* After that the Mouse presses */
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
	{
		if (mouse_map.at(MOUSE_BUTTON_LEFT).input != UserInput::NONE)
		{
			event_system.AddEvent<UserInputEvent>({ mouse_map.at(MOUSE_BUTTON_LEFT).input });
		}
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		if (mouse_map.at(MOUSE_BUTTON_RIGHT).input != UserInput::NONE)
		{
			event_system.AddEvent<UserInputEvent>({ mouse_map.at(MOUSE_BUTTON_RIGHT).input });
		}
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
	{
		if (mouse_map.at(MOUSE_BUTTON_MIDDLE).input != UserInput::NONE)
		{
			event_system.AddEvent<UserInputEvent>({ mouse_map.at(MOUSE_BUTTON_MIDDLE).input });
		}
	}
	if (IsMouseButtonPressed(MOUSE_BUTTON_SIDE))
	{
		if (mouse_map.at(MOUSE_BUTTON_SIDE).input != UserInput::NONE)
		{
			event_system.AddEvent<UserInputEvent>({ mouse_map.at(MOUSE_BUTTON_SIDE).input });
		}
	}
	if (IsMouseButtonPressed(MOUSE_BUTTON_EXTRA))
	{
		if (mouse_map.at(MOUSE_BUTTON_EXTRA).input != UserInput::NONE)
		{
			event_system.AddEvent<UserInputEvent>({ mouse_map.at(MOUSE_BUTTON_EXTRA).input });
		}
	}
	if (IsMouseButtonPressed(MOUSE_BUTTON_FORWARD))
	{
		if (mouse_map.at(MOUSE_BUTTON_FORWARD).input != UserInput::NONE)
		{
			event_system.AddEvent<UserInputEvent>({ mouse_map.at(MOUSE_BUTTON_FORWARD).input });
		}
	}
	if (IsMouseButtonPressed(MOUSE_BUTTON_BACK))
	{
		if (mouse_map.at(MOUSE_BUTTON_BACK).input != UserInput::NONE)
		{
			event_system.AddEvent<UserInputEvent>({ mouse_map.at(MOUSE_BUTTON_BACK).input });
		}
	}

	/* Get Gamepad Buttons */

	key = GetGamepadButtonPressed();

	if (gamepad_map.at(key).input != UserInput::NONE)
	{
		event_system.AddEvent<UserInputEvent>({ gamepad_map.at(key).input });
	}

}

void Mupfel::InputManager::UpdateCursor()
{
}

UserInputEvent::UserInputEvent() : input(UserInput::NONE)
{
}

Mupfel::UserInputEvent::UserInputEvent(UserInput in_input) : input(in_input)
{
}
