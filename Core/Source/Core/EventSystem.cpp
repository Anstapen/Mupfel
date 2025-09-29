#include "EventSystem.h"

Mupfel::EventSystem::EventSystem() : current(0), next(1)
{
}

void Mupfel::EventSystem::Update()
{
	/* Tick is over, clear the current EventBuffers */
	for (const auto& e : event_map[current])
	{
		e.second->Clear();
	}

	/* Update the index */
	next = current;
	current = (current + 1) & 0x1;
}
