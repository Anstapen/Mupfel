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

struct CircleData
{
	float radius = 1.0f;
};

struct BoxData
{
	float _pad0 = 0.0f;
	float half_width = 0.5f;
	float half_height = 0.5f;
};

struct CapsuleData
{
	float radius = 1.0f;
	float center1_x = 0.0f;
	float center1_y = 0.0f;
	float center2_x = 1.0f;
	float center2_y = 0.0f;
};

union ShapeData
{
	ShapeData() : box() {}

	CircleData	circle;
	BoxData		box;
	CapsuleData capsule;
};
}
