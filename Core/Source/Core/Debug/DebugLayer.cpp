#include "DebugLayer.h"
#include "Core/Application.h"
#include "Core/Profiler.h"
#include "ECS/Components/Body.h"
#include "ECS/Components/Collider.h"
#include "ECS/Components/Sensor.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/View.h"
#include "Renderer/CameraMath.h"
#include "Renderer/Renderer.h"
#include <algorithm>
#include <format>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#include "imgui.h"

void Mupfel::DebugLayer::OnInit() {}

void Mupfel::DebugLayer::OnUpdate(double timestep) { (void)timestep; }

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

	if (ImGui::CollapsingHeader("Camera Controls"))
	{
		DrawCameraControls();
	}
	static bool drawEntityColliders = false;
	ImGui::Checkbox("Draw Entity colliders", &drawEntityColliders);

	if (drawEntityColliders)
	{
		ProfilingSample prof("Debug: Drawing Colliders");
		DrawEntityColliders();
	}

	if (ImGui::CollapsingHeader("Performance Metrics"))
	{
		DrawPerformanceMetrics();
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

	/* Draw all existing colliders in red. */
	for (auto [e, t, c, b] : Application::GetCurrentRegistry().view<Transform, Collider, Body>())
	{

		switch (c.shape)
		{
		case ColliderShape::Circle:
			DrawCircleCollider(t, c);
			break;
		case ColliderShape::Capsule:
			DrawCapsuleCollider(t, c);
			break;
		default:
			DrawBoxCollider(t, c);
			break;
		}
	}

	/* Draw all existing sensors in light blue. */
	for (auto [e, t, s, b] : Application::GetCurrentRegistry().view<Transform, Sensor, Body>())
	{
		switch (s.shape)
		{
		case ColliderShape::Circle:
			DrawCircleSensor(t, s);
			break;
		case ColliderShape::Capsule:
			DrawCapsuleSensor(t, s);
			break;
		default:
			DrawBoxSensor(t, s);
			break;
		}
	}
}

void Mupfel::DebugLayer::DrawCircleCollider(Transform& t, Collider& c)
{

	const glm::vec3 centre(t.pos_x + c.offset_x, t.pos_y + c.offset_y, t.pos_z);

	const glm::vec2 p = ToPixels(centre);

	const float radius = glm::length(ToPixels(centre + glm::vec3(0.0f, c.data.circle.radius, 0.0f)) - p);
	glm::vec4	red = {1.0f, 0.0f, 0.0f, 1.0f};

	Application::Get().renderer->geoRenderer->Circle({p.x, p.y}, radius, red, 2);
}

void Mupfel::DebugLayer::DrawBoxCollider(Transform& t, Collider& c)
{
	const glm::vec3 centre(t.pos_x + c.offset_x, t.pos_y + c.offset_y, t.pos_z);

	const glm::vec2 p = ToPixels(centre);

	const float half_w = glm::length(ToPixels(centre + glm::vec3(c.data.box.half_width, 0.0f, 0.0f)) - p);
	const float half_h = glm::length(ToPixels(centre + glm::vec3(0.0f, c.data.box.half_height, 0.0f)) - p);
	glm::vec4	red = {1.0f, 0.0f, 0.0f, 1.0f};

	Application::Get().renderer->geoRenderer->Rectangle(
		{p.x - half_w, p.y - half_h}, half_w * 2.0f, half_h * 2.0f, red, 2);
}

void Mupfel::DebugLayer::DrawCapsuleCollider(Transform& t, Collider& c)
{
	const glm::vec3 centre(t.pos_x + c.offset_x, t.pos_y + c.offset_y, t.pos_z);

	const glm::vec2 p = ToPixels(centre);

	const float		radius = glm::length(ToPixels(centre + glm::vec3(0.0f, c.data.circle.radius, 0.0f)) - p);
	const glm::vec2 centre1_pixels =
		ToPixels(glm::vec3(centre.x + c.data.capsule.center1_x, centre.y + c.data.capsule.center1_y, centre.z));
	const glm::vec2 centre2_pixels =
		ToPixels(glm::vec3(centre.x + c.data.capsule.center2_x, centre.y + c.data.capsule.center2_y, centre.z));

	const glm::vec2 bottom_left =
		ToPixels(glm::vec3(centre.x + c.data.capsule.center2_x, centre.y + c.data.capsule.center2_y, centre.z));

	glm::vec4 red = {1.0f, 0.0f, 0.0f, 1.0f};

	Application::Get().renderer->geoRenderer->Circle({centre1_pixels.x, centre1_pixels.y}, radius, red, 2);
	Application::Get().renderer->geoRenderer->Circle({centre2_pixels.x, centre2_pixels.y}, radius, red, 2);
	Application::Get().renderer->geoRenderer->Line(
		{centre1_pixels.x, centre1_pixels.y + radius}, {centre2_pixels.x, centre2_pixels.y + radius}, red);
	Application::Get().renderer->geoRenderer->Line(
		{centre1_pixels.x, centre1_pixels.y - radius}, {centre2_pixels.x, centre2_pixels.y - radius}, red);
}

void Mupfel::DebugLayer::DrawCircleSensor(Transform& t, Sensor& s)
{
	glm::vec4		light_blue = {0.212f, 0.906f, 1.0f, 1.0f};
	const glm::vec3 centre(t.pos_x + s.offset_x, t.pos_y + s.offset_y, t.pos_z);

	const glm::vec2 p = ToPixels(centre);

	const float radius = glm::length(ToPixels(centre + glm::vec3(0.0f, s.data.circle.radius, 0.0f)) - p);

	Application::Get().renderer->geoRenderer->Circle({p.x, p.y}, radius, light_blue, 2);
}

void Mupfel::DebugLayer::DrawBoxSensor(Transform& t, Sensor& s)
{
	glm::vec4		light_blue = {0.212f, 0.906f, 1.0f, 1.0f};
	const glm::vec3 centre(t.pos_x + s.offset_x, t.pos_y + s.offset_y, t.pos_z);

	const glm::vec2 p = ToPixels(centre);

	const float half_w = glm::length(ToPixels(centre + glm::vec3(s.data.box.half_width, 0.0f, 0.0f)) - p);
	const float half_h = glm::length(ToPixels(centre + glm::vec3(0.0f, s.data.box.half_height, 0.0f)) - p);

	Application::Get().renderer->geoRenderer->Rectangle(
		{p.x - half_w, p.y - half_h}, half_w * 2.0f, half_h * 2.0f, light_blue, 2);
}

void Mupfel::DebugLayer::DrawCapsuleSensor(Transform& t, Sensor& s)
{
	glm::vec4		light_blue = {0.212f, 0.906f, 1.0f, 1.0f};
	const glm::vec3 centre(t.pos_x + s.offset_x, t.pos_y + s.offset_y, t.pos_z);

	const glm::vec2 p = ToPixels(centre);

	const float		radius = glm::length(ToPixels(centre + glm::vec3(0.0f, s.data.circle.radius, 0.0f)) - p);
	const glm::vec2 centre1_pixels =
		ToPixels(glm::vec3(centre.x + s.data.capsule.center1_x, centre.y + s.data.capsule.center1_y, centre.z));
	const glm::vec2 centre2_pixels =
		ToPixels(glm::vec3(centre.x + s.data.capsule.center2_x, centre.y + s.data.capsule.center2_y, centre.z));

	Application::Get().renderer->geoRenderer->Circle({centre1_pixels.x, centre1_pixels.y}, radius, light_blue, 2);
	Application::Get().renderer->geoRenderer->Circle({centre2_pixels.x, centre2_pixels.y}, radius, light_blue, 2);
	Application::Get().renderer->geoRenderer->Line(
		{centre1_pixels.x, centre1_pixels.y + radius}, {centre2_pixels.x, centre2_pixels.y + radius}, light_blue);
	Application::Get().renderer->geoRenderer->Line(
		{centre1_pixels.x, centre1_pixels.y - radius}, {centre2_pixels.x, centre2_pixels.y - radius}, light_blue);
}

void Mupfel::DebugLayer::UpdateMVP()
{
	int32_t		  width = Application::GetCurrentRenderWidth();
	int32_t		  height = Application::GetCurrentRenderHeight();
	const Camera& cam = Application::GetCurrentSceneCamera();

	view = CameraMath::View(cam);

	proj = CameraMath::Projection(cam, static_cast<float>(width), static_cast<float>(height));
}

glm::vec2 Mupfel::DebugLayer::ToPixels(glm::vec3 world_pos)
{
	const float screen_w = static_cast<float>(Application::GetCurrentRenderWidth());
	const float screen_h = static_cast<float>(Application::GetCurrentRenderHeight());

	const glm::vec4 clip = proj * view * glm::vec4(world_pos, 1.0f);
	const glm::vec2 ndc = glm::vec2(clip) / clip.w;
	return glm::vec2((ndc.x * 0.5f + 0.5f) * screen_w, (ndc.y * 0.5f + 0.5f) * screen_h);
}
