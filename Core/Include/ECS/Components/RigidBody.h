#pragma once
#include <cstdint>

namespace Mupfel
{
/**
 * The types of body an entity can have.
 */
enum class BodyType : uint8_t
{
	Static,
	Kinematic,
	Dynamic
};

struct RigidBody
{
	BodyType type = BodyType::Static;
	float	 gravity_scale = 1.0f;
	float	 linear_damping = 0.0f;
	float	 angular_damping = 0.0f;
	bool	 fixed_rotation = false;
	bool	 is_bullet = false;
	bool	 allow_sleep = true;
};
} // namespace Mupfel
