#pragma once
#include <optional>

namespace Mupfel
{

/** A pixel on the render surface. Origin is the top-left corner, +Y points down. */
struct ScreenPoint
{
	float x = 0.0f;
	float y = 0.0f;
};

/** A point in world space. Z is up. */
struct WorldPoint
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

/** A displacement in pixels on the render surface. +Y points down. */
struct ScreenVector
{
	float x = 0.0f;
	float y = 0.0f;
};

/** A displacement in world space. Z is up. */
struct WorldVector
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

class Camera
{
public:
	Camera();

	[[nodiscard]] ScreenPoint WorldToScreen(float world_x, float world_y, float world_z = 0.0f) const;

	[[nodiscard]] std::optional<WorldPoint> ScreenToWorld(float screen_x, float screen_y, float plane_z = 0.0f) const;

	[[nodiscard]] ScreenVector WorldToScreenVector(float world_dx, float world_dy, float world_dz = 0.0f) const;

	[[nodiscard]] std::optional<WorldVector> ScreenToWorldVector(float screen_dx, float screen_dy) const;

	float									yaw;
	float									pitch;
	float									distance;
	float									target_x;
	float									target_y;
	float									target_z;
};
} // namespace Mupfel
