#pragma once
#include <cstdint>

namespace Mupfel
{
	enum class ColliderShape : uint8_t
	{
		Box,
		Circle,
		Capsule
	};

struct Collider
{
	ColliderShape shape = ColliderShape::Box;

	float half_width = 0.5f;
	float half_height = 0.5f;
	
	float offset_x = 0.0f;
	float offset_y = 0.0f;

	float density = 1.0f;
	float friction = 0.6f;
	float restitution = 0.0f;

	bool is_sensor = false;
	bool report_contacts = false;
	bool report_hit_events = false;
	bool report_sensor_events = false;

	uint64_t category = 1;
	uint64_t mask = ~uint64_t{0};
};

} // namespace Mupfel
