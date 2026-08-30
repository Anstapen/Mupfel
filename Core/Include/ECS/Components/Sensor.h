#pragma once
#include <cstdint>

namespace Mupfel
{
enum class SensorShape : uint8_t
{
	Box,
	Circle,
	Capsule
};

struct Sensor
{
	SensorShape shape = SensorShape::Box;

	float half_width = 0.5f;
	float half_height = 0.5f;

	float offset_x = 0.0f;
	float offset_y = 0.0f;

	bool report_events = false;

	uint64_t category = 1;
	uint64_t mask = ~uint64_t{0};
};
} // namespace Mupfel
