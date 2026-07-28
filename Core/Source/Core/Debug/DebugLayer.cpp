#include "DebugLayer.h"
#include "Core/Application.h"
#include "Core/Profiler.h"
#include "ECS/Components/Collider.h"
#include "ECS/Components/Movement.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/View.h"
#include <format>
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
	auto height = static_cast<float>(Application::GetCurrentRenderHeight());
	auto width =  static_cast<float>(Application::GetCurrentRenderWidth()) / 4.0f;
	float window_x = static_cast<float>(Application::GetCurrentRenderWidth()) - width;
	ImGui::SetWindowPos({window_x, 0.0f});
	ImGui::SetWindowSize({width, height});

	//ImGui::ShowDemoWindow();
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
		std::stable_sort(local.begin(), local.end(), [](auto const& a, auto const& b) { return a.id < b.id; });

		std::string t;
		uint32_t offset = 200;
		for (const auto& s : local)
		{
			std::string indent(s.depth * 2, ' ');
			double elapsed_ms = (s.end_time - s.start_time) * 1000.0f;
			t = std::vformat("{}{}: {:.0f}ms", std::make_format_args(indent, s.name, elapsed_ms));
			ImGui::Text(t.c_str());
			offset += 20;
		}
	}

}
