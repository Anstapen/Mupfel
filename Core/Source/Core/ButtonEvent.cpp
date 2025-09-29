#include "ButtonEvent.h"

Mupfel::ButtonEvent::ButtonEvent() : key(Key::W), state(KeyState::PRESSED)
{
}

Mupfel::ButtonEvent::ButtonEvent(Key in_key, KeyState in_key_state) : key(in_key), state(in_key_state)
{
}
