#pragma once
#include "ECS/Entity.h"
#include "Event.h"

namespace Mupfel
{
class CollisionBeganEvent : public Event
{
public:
	CollisionBeganEvent(Entity in_a, Entity in_b) : a(in_a), b(in_b) {}

	Entity a, b;
};

class CollisionEndedEvent : public Event
{
public:
	CollisionEndedEvent(Entity in_a, Entity in_b) : a(in_a), b(in_b) {}

	Entity a, b;
};

class SensorEnteredEvent : public Event
{
public:
	SensorEnteredEvent(Entity in_sensor, Entity in_visitor) : sensor(in_sensor), visitor(in_visitor) {}

	Entity sensor, visitor;
};

class SensorExitedEvent : public Event
{
public:
	SensorExitedEvent(Entity in_sensor, Entity in_visitor) : sensor(in_sensor), visitor(in_visitor) {}

	Entity sensor, visitor;
};
} // namespace Mupfel
