#pragma once
#include <cstdint>

enum class ShapeType : uint32_t
{
	None,
	Circle,
	AABB,
	OBB,
	Capsule,
	Point,
	Ray,
	Segment,
	Sector,
	ConvexPolygon,

	/* Total count of the defined enum values. */
	COUNT
};

