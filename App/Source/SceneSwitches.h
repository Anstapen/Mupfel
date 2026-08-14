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

class SwitchToGravityTestEvent : public Mupfel::Event
{
public:
	SwitchToGravityTestEvent() {};
};
