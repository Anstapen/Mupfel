#include "Camera.h"
#include "Core/Application.h"
#include "CameraMath.h"

using namespace Mupfel;

Camera::Camera()
	: yaw(glm::radians(-90.0f)), pitch(glm::radians(89.9f)), distance(25.0f), target_x(0.0f), target_y(0.0f),
	  target_z(0.0f)
{
}

ScreenPoint Camera::WorldToScreen(float world_x, float world_y, float world_z) const
{
	const float width = static_cast<float>(Application::GetCurrentRenderWidth());
	const float height = static_cast<float>(Application::GetCurrentRenderHeight());

	if (width <= 0.0f || height <= 0.0f)
	{
		return {};
	}

	const glm::vec4 clip = CameraMath::Projection(*this, width, height) * CameraMath::View(*this) *
						   glm::vec4(world_x, world_y, world_z, 1.0f);

	/* Make sure w is one. */
	const glm::vec3 ndc = glm::vec3(clip) / clip.w;

	return {(ndc.x * 0.5f + 0.5f) * width, (0.5f - ndc.y * 0.5f) * height};
}

std::optional<WorldPoint> Mupfel::Camera::ScreenToWorld(float screen_x, float screen_y, float plane_z) const
{
	const float width = static_cast<float>(Application::GetCurrentRenderWidth());
	const float height = static_cast<float>(Application::GetCurrentRenderHeight());

	if (width <= 0.0f || height <= 0.0f)
	{
		return std::nullopt;
	}

	const glm::vec3 forward = CameraMath::Forward(*this);

	/* If the pitch is 0, the ray does not hit the x,y plane. */
	if (glm::abs(forward.z) < 1e-6f)
	{
		return std::nullopt;
	}

	const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 0.0f, 1.0f)));
	const glm::vec3 up = glm::cross(right, forward);
	const glm::vec2 half = CameraMath::HalfExtents(*this, width, height);
	const float		offset_right = ((screen_x / width) * 2.0f - 1.0f) * half.x;
	const float		offset_up = (1.0f - (screen_y / height) * 2.0f) * half.y;
	const glm::vec3 origin = glm::vec3(target_x, target_y, target_z) + right * offset_right + up * offset_up;
	const glm::vec3 hit = origin + forward * ((plane_z - origin.z) / forward.z);

	return WorldPoint{hit.x, hit.y, plane_z};
}

ScreenVector Camera::WorldToScreenVector(float world_dx, float world_dy, float world_dz) const
{
	const float width = static_cast<float>(Application::GetCurrentRenderWidth());
	const float height = static_cast<float>(Application::GetCurrentRenderHeight());

	if (width <= 0.0f || height <= 0.0f)
	{
		return {};
	}

	const glm::vec4 clip = CameraMath::Projection(*this, width, height) * CameraMath::View(*this) *
						   glm::vec4(world_dx, world_dy, world_dz, 0.0f);

	return {clip.x * 0.5f * width, -clip.y * 0.5f * height};
}

std::optional<WorldVector> Camera::ScreenToWorldVector(float screen_dx, float screen_dy) const
{
	const float width = static_cast<float>(Application::GetCurrentRenderWidth());
	const float height = static_cast<float>(Application::GetCurrentRenderHeight());

	if (width <= 0.0f || height <= 0.0f)
	{
		return std::nullopt;
	}

	const glm::vec3 forward = CameraMath::Forward(*this);

	/* If the pitch is 0, the ray does not hit the x,y plane. */
	if (glm::abs(forward.z) < 1e-6f)
	{
		return std::nullopt;
	}

	const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 0.0f, 1.0f)));
	const glm::vec3 up = glm::cross(right, forward);
	const glm::vec2 half = CameraMath::HalfExtents(*this, width, height);

	const glm::vec3 offset =
		right * ((screen_dx / width) * 2.0f * half.x) - up * ((screen_dy / height) * 2.0f * half.y);

	return WorldVector{
		offset.x - forward.x * (offset.z / forward.z), offset.y - forward.y * (offset.z / forward.z), 0.0f};
}
