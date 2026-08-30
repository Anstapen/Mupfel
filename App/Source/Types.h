#pragma once

enum ColliderType
{
	Player = 1 << 0,
	GroundObject = 1 << 2,
	WallObject = 1 << 3,
	Furniture = 1 << 4
};