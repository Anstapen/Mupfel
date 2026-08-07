#pragma once

namespace Mupfel
{
class Camera
{
public:
	Camera();
	float yaw;
	float pitch;
	float distance;
	float target_x;
	float target_y;
	float target_z;
};
} // namespace Mupfel
