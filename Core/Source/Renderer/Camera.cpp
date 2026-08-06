#include "Camera.h"
#include "glm/glm.hpp"

Mupfel::Camera::Camera()
	: yaw(glm::radians(-90.0f)), pitch(glm::radians(45.0f)), distance(25.0f), target_x(0.0f), target_y(0.0f),
	  target_z(0.0f)
{
}
