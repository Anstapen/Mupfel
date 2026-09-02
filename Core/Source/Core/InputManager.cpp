#include "InputManager.h"
#include "Application.h"

using namespace Mupfel;

InputManager::InputManager(EventSystem& evt_system, Mode in_mode) : event_system(evt_system), current_mode(in_mode)
{
	/* First, reset all mappings */
	std::function<void(EventSystem&)> default_emitter = nullptr;
	std::array<Binding, 3> default_bindings = {default_emitter, default_emitter, default_emitter};
	keyboard_map.fill(default_bindings);
	mouse_map.fill(default_bindings);
	gamepad_map.fill(default_bindings);

	/*
		For now, just manually set the default mappings.
		In the future, we will have some kind of loader that loads
		saved mappings from somewhere (probably a local json file?).
	*/

	MapKeyboardButton<UserInputEvent>(Key::KEY_F, KeyAction::PRESSED, {UserInput::WINDOW_FULLSCREEN, KeyAction::NONE});
	MapKeyboardButton<UserInputEvent>(Key::KEY_F1, KeyAction::PRESSED, {UserInput::TOGGLE_DEBUG_MODE, KeyAction::NONE});
	MapMouseButton<UserInputEvent>(
		MouseButton::MOUSE_BUTTON_LEFT, KeyAction::RELEASED, {UserInput::LEFT_MOUSE_CLICK, KeyAction::RELEASED});
	MapMouseButton<UserInputEvent>(
		MouseButton::MOUSE_BUTTON_LEFT, KeyAction::PRESSED, {UserInput::LEFT_MOUSE_CLICK, KeyAction::PRESSED});
	MapMouseButton<UserInputEvent>(
		MouseButton::MOUSE_BUTTON_RIGHT, KeyAction::RELEASED, {UserInput::RIGHT_MOUSE_CLICK, KeyAction::RELEASED});
	MapMouseButton<UserInputEvent>(
		MouseButton::MOUSE_BUTTON_RIGHT, KeyAction::PRESSED, {UserInput::RIGHT_MOUSE_CLICK, KeyAction::PRESSED});
}

double Mupfel::InputManager::GetCurrentCursorX() const { return current_mouse_pos_x; }

double Mupfel::InputManager::GetCurrentCursorY() const { return current_mouse_pos_y; }

bool Mupfel::InputManager::CheckUserInput(UserInput ui, KeyAction a) const
{
	for (const auto& evt : event_system.GetEvents<Mupfel::UserInputEvent>())
	{
		if (evt.input == ui && evt.action == a)
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

	auto action_idx = static_cast<size_t>(action);

	if (action_idx > 0)
	{
		action_idx -= 1;
	}

	if (action_idx >= keyboard_map[key_index].size())
	{
		return;
	}

	/* Check if the key is mapped to a function. */
	if (keyboard_map[key_index][action_idx].emitter != nullptr)
	{
		keyboard_map[key_index][action_idx].emitter(event_system);
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

	auto action_idx = static_cast<size_t>(action);

	if (action_idx > 0)
	{
		action_idx -= 1;
	}

	if (action_idx >= keyboard_map[mb_index].size())
	{
		return;
	}

	/* Check if the key is mapped to a function. */
	if (mouse_map[mb_index][action_idx].emitter != nullptr)
	{
		mouse_map[mb_index][action_idx].emitter(event_system);
	}
}

void Mupfel::InputManager::UpdateScrollWheel(double offset_x, double offset_y)
{
	(void)offset_x;
	/* For now, we only support scrolling on the y-axis. */
	if (offset_y < 0.0)
	{
		event_system.AddEvent<UserInputEvent>({UserInput::SCROLLWHEEL_DOWN, KeyAction::NONE});
	}
	else if (offset_y > 0.0)
	{
		event_system.AddEvent<UserInputEvent>({UserInput::SCROLLWHEEL_UP, KeyAction::NONE});
	}
	
}
