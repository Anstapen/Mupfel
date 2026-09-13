#pragma once
#include "Renderer/Camera.h"
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Mupfel::CameraMath
{
/** Unit vector from the eye towards the target. */
inline glm::vec3 Forward(const Camera& cam)
{
	return -glm::vec3(
		glm::cos(cam.pitch) * glm::cos(cam.yaw), glm::cos(cam.pitch) * glm::sin(cam.yaw), glm::sin(cam.pitch));
}

inline glm::vec2 HalfExtents(const Camera& cam, float width, float height)
{
	const float half_height = cam.distance * glm::tan(glm::radians(45.0f) * 0.5f);
	return {half_height * (width / height), half_height};
}

/** Position of the eye. */
inline glm::vec3 Eye(const Camera& cam)
{
	return glm::vec3(cam.target_x, cam.target_y, cam.target_z) - cam.distance * Forward(cam);
}

/** View matrix. */
inline glm::mat4 View(const Camera& cam)
{
	return glm::lookAt(Eye(cam), glm::vec3(cam.target_x, cam.target_y, cam.target_z), glm::vec3(0.0f, 0.0f, 1.0f));
}

/** Projection matrix. */
inline glm::mat4 Projection(const Camera& cam, float width, float height)
{
	const glm::vec2 half = HalfExtents(cam, width, height);

	glm::mat4 proj = glm::ortho(-half.x, half.x, -half.y, half.y, 0.1f, 500.0f);

	return proj;
}

} // namespace Mupfel::CameraMath
