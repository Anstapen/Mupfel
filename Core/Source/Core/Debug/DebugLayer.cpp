#include "DebugLayer.h"
#include "Core/Application.h"
#include "Core/Profiler.h"
#include "ECS/Components/Collider.h"
#include "ECS/Components/Movement.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/View.h"
#include "Renderer/Renderer.h"
#include <algorithm>
#include <format>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#include "imgui.h"

void Mupfel::DebugLayer::OnInit() {}

void Mupfel::DebugLayer::OnUpdate(double timestep)
{
#if 0
	if (single_stepping && IsKeyPressed(KEY_SPACE))
	{
		Application::PhysicsStep();
	}
#endif
}

void Mupfel::DebugLayer::OnRender()
{
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;
	if (!ImGui::Begin("DebugInfo", nullptr, flags))
	{
		ImGui::End();
		return;
	}
	auto  height = static_cast<float>(Application::GetCurrentRenderHeight());
	auto  width = static_cast<float>(Application::GetCurrentRenderWidth()) / 4.0f;
	float window_x = static_cast<float>(Application::GetCurrentRenderWidth()) - width;
	ImGui::SetWindowPos({window_x, 0.0f});
	ImGui::SetWindowSize({width, height});

	// ImGui::ShowDemoWindow();
	if (ImGui::CollapsingHeader("Performance Metrics"))
	{
		DrawPerformanceMetrics();
	}
	if (ImGui::CollapsingHeader("Camera Controls"))
	{
		DrawCameraControls();
	}
	static bool drawEntityColliders = false;
	ImGui::Checkbox("Draw Entity colliders", &drawEntityColliders);

	if (drawEntityColliders)
	{
		DrawEntityColliders();
	}

	ImGui::End();
}

void Mupfel::DebugLayer::DrawPerformanceMetrics()
{
	/* Print the Profiling Samples */
	const std::vector<ProfilingSample>& samples = Mupfel::Profiler::GetCurrentSamples();

	std::vector<ProfilingSample> local(samples.begin(), samples.end());

	if (!local.empty())
	{
		// Sortiere stabil nach Startzeit (aufsteigend)
		std::ranges::stable_sort(local, {}, &ProfilingSample::id);

		std::string t;
		uint32_t	offset = 200;
		for (const auto& s : local)
		{
			std::string indent(s.depth * 2, ' ');
			double		elapsed_ms = (s.end_time - s.start_time) * 1000.0f;
			t = std::vformat("{}{}: {:.0f}ms", std::make_format_args(indent, s.name, elapsed_ms));
			ImGui::Text("%s", t.c_str());
			offset += 20;
		}
	}
}

void Mupfel::DebugLayer::DrawCameraControls()
{
	Camera& current_cam = Application::GetCurrentSceneCamera();

	float current_yaw = glm::degrees(current_cam.yaw);
	float current_pitch = glm::degrees(current_cam.pitch);
	ImGui::SliderFloat("Yaw", &current_yaw, -180.0f, 180.0f);
	ImGui::SliderFloat("Pitch", &current_pitch, 0.0f, 90.0f);
	ImGui::SliderFloat("Distance", &current_cam.distance, 0.0f, 90.0f);
	current_cam.yaw = glm::radians(current_yaw);
	current_cam.pitch = glm::radians(current_pitch);
}

void Mupfel::DebugLayer::DrawEntityColliders()
{
	UpdateMVP();

	const float screen_w = static_cast<float>(Application::GetCurrentRenderWidth());
	const float screen_h = static_cast<float>(Application::GetCurrentRenderHeight());

	/* The position in the transform component is a world position, we need to convert it to a screen position
	 * (pixels). */
	auto to_pixels = [&](glm::vec3 world_pos)
	{
		const glm::vec4 clip = proj * view * glm::vec4(world_pos, 1.0f);
		const glm::vec2 ndc = glm::vec2(clip) / clip.w;
		return glm::vec2((ndc.x * 0.5f + 0.5f) * screen_w, (ndc.y * 0.5f + 0.5f) * screen_h);
	};

	for (auto [e, t, c] : Application::GetCurrentRegistry().view<Transform, Collider>())
	{
		const glm::vec3 centre(t.pos_x + c.offset_x, t.pos_y + c.offset_y, t.pos_z);

		const glm::vec2 p = to_pixels(centre);

		/* The half extents also need to be converted to screen pixels. */
		const float half_w = glm::length(to_pixels(centre + glm::vec3(c.half_width, 0.0f, 0.0f)) - p);
		const float half_h = glm::length(to_pixels(centre + glm::vec3(0.0f, c.half_height, 0.0f)) - p);

		/* Depending on the collider shape, draw a primitive in red. */
		glm::vec4 red = {1.0f, 0.0f, 0.0f, 1.0f};
		switch (c.shape)
		{
		case ColliderShape::Circle:
			Application::Get().renderer->geoRenderer->Circle({p.x, p.y}, half_w, red, 2);
			break;
		default:
			/* Rectangle positions by its lower-left corner, not its centre. */
			Application::Get().renderer->geoRenderer->Rectangle(
				{p.x - half_w, p.y - half_h}, half_w * 2.0f, half_h * 2.0f, red, 2);
			break;
		}
		
		
	}
}

void Mupfel::DebugLayer::UpdateMVP()
{
	int32_t width = Application::GetCurrentRenderWidth();
	int32_t height = Application::GetCurrentRenderHeight();
	Camera	cam = Application::GetCurrentSceneCamera();

	glm::vec3 cameraTarget = glm::vec3(cam.target_x, cam.target_y, cam.target_z);

	glm::vec3 eye = cameraTarget + cam.distance * glm::vec3(
													  glm::cos(cam.pitch) * glm::cos(cam.yaw),
													  glm::cos(cam.pitch) * glm::sin(cam.yaw), glm::sin(cam.pitch));

	view = lookAt(eye, cameraTarget, glm::vec3(0.0f, 0.0f, 1.0f));
	const float aspect = static_cast<float>(width) / static_cast<float>(height);
	const float half_height = cam.distance * glm::tan(glm::radians(45.0f) * 0.5f);
	const float half_width = half_height * aspect;

	proj = glm::ortho(-half_width, half_width, -half_height, half_height, 0.1f, 500.0f);

	proj[1][1] *= -1;
}
