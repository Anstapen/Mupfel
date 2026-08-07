#pragma once
#include "Core/Event.h"

class SwitchToMainMenuEvent : public Mupfel::Event
{
public:
	SwitchToMainMenuEvent() {};
};

class SwitchToLevelEvent : public Mupfel::Event
{
public:
	SwitchToLevelEvent() {};
};