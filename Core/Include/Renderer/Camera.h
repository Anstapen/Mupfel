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

class Camera
{
public:
	Camera();

	[[nodiscard]] ScreenPoint WorldToScreen(float world_x, float world_y, float world_z = 0.0f) const;

	[[nodiscard]] std::optional<WorldPoint> ScreenToWorld(float screen_x, float screen_y, float plane_z = 0.0f) const;

	float									yaw;
	float									pitch;
	float									distance;
	float									target_x;
	float									target_y;
	float									target_z;
};
} // namespace Mupfel
