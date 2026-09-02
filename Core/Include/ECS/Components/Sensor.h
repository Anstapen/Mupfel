#pragma once
#include <cstdint>
#include "Common.h"

namespace Mupfel
{

struct Sensor
{
	Sensor() {}

	inline void SetCircle(float radius)
	{
		shape = ColliderShape::Circle;
		data.circle.radius = radius;
	}

	inline void SetBox(float width, float height)
	{
		shape = ColliderShape::Box;
		data.box.half_width = width / 2.0f;
		data.box.half_height = height / 2.0f;
	}

	inline void SetCapsule(float center1_x, float center1_y, float center2_x, float center2_y, float radius)
	{
		shape = ColliderShape::Capsule;
		data.capsule.radius = radius;
		data.capsule.center1_x = center1_x;
		data.capsule.center1_y = center1_y;
		data.capsule.center2_x = center2_x;
		data.capsule.center2_y = center2_y;
	}

	ColliderShape shape = ColliderShape::Box;

	ShapeData data;

	float offset_x = 0.0f;
	float offset_y = 0.0f;

	bool report_events = false;

	uint64_t category = 1;
	uint64_t mask = ~uint64_t{0};
};
} // namespace Mupfel
