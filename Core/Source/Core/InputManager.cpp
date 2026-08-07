#include "InputManager.h"
#include "Application.h"

using namespace Mupfel;

InputManager::InputManager(EventSystem& evt_system, Mode in_mode) : event_system(evt_system), current_mode(in_mode)
{
	/* First, reset all mappings */
	Binding default_binding;
	default_binding.emitter = [](EventSystem& es) { es.AddEvent<UserInputEvent>({UserInput::NONE, KeyAction::NONE}); };
	keyboard_map.fill(default_binding);
	mouse_map.fill(default_binding);
	gamepad_map.fill(default_binding);

	/* Set the GLFW key callback */

	/*
		For now, just manually set the default mappings.
		In the future, we will have some kind of loader that loads
		saved mappings from somewhere (probably a local json file?).
	*/

	MapKeyboardButton<UserInputEvent>(Key::KEY_F, KeyAction::PRESSED, {UserInput::WINDOW_FULLSCREEN, KeyAction::NONE});
	MapKeyboardButton<UserInputEvent>(Key::KEY_F1, KeyAction::PRESSED, {UserInput::TOGGLE_DEBUG_MODE, KeyAction::NONE});
	MapMouseButton<UserInputEvent>(
		MouseButton::MOUSE_BUTTON_LEFT, KeyAction::RELEASED, {UserInput::LEFT_MOUSE_CLICK, KeyAction::RELEASED});
}

double Mupfel::InputManager::GetCurrentCursorX() const { return current_mouse_pos_x; }

double Mupfel::InputManager::GetCurrentCursorY() const { return current_mouse_pos_y; }

bool Mupfel::InputManager::CheckUserInput(UserInput ui) const
{
	for (const auto& evt : event_system.GetEvents<Mupfel::UserInputEvent>())
	{
		if (evt.input == ui)
		{
			return true;
		}
	}
	return false;
}

void Mupfel::InputManager::UpdateCursor(double new_pos_x, double new_pos_y)
{
	current_mouse_pos_x = new_pos_x;
	current_mouse_pos_y = new_pos_y;
	event_system.AddEvent<UserInputEvent>({UserInput::CURSOR_POS_CHANGED, KeyAction::NONE});
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
	if (HasFlag(keyboard_map[key_index].actionMask, action))
	{
		keyboard_map[key_index].emitter(event_system);
	}
}

void Mupfel::InputManager::MouseButtonPressed(MouseButton b, KeyAction action)
{
	if (current_mode != Mode::MOUSE_KEYBOARD)
	{
		return;
	}

	auto mb_index = static_cast<uint32_t>(b);

	/* Check if the key is known. */
	if (mb_index >= mouse_map.size())
	{
		return;
	}

	/* Check if the key is mapped to a function. */
	if (HasFlag(mouse_map[mb_index].actionMask, action))
	{
		mouse_map[mb_index].emitter(event_system);
	}
}
