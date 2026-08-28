#pragma once
#include <cstdint>

namespace Mupfel
{
enum class BodyType : uint8_t
{
	Static,
	Kinematic,
	Dynamic
};

struct Body
{
	BodyType type = BodyType::Static;
	float	 velocity_x = 0.0f;
	float	 velocity_y = 0.0f;
	float	 angular_velocity = 0.0f;
	float	 gravity_scale = 1.0f;
	float	 linear_damping = 0.0f;
	float	 angular_damping = 0.0f;
	bool	 fixed_rotation = false;
	bool	 is_bullet = false;
	bool	 allow_sleep = true;
};
} // namespace Mupfel