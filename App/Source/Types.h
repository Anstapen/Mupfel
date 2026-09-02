#pragma once
#include <cstdint>

namespace ColliderType
{
enum : uint64_t
{
	Player = 1ULL << 0,
	GroundObject = 1ULL << 1,
	WallObject = 1ULL << 2,
	Furniture = 1ULL << 3
};
}
