#include "DebugLayer.h"
#include "Core/Application.h"
#include "Renderer/Rectangle.h"
#include "Renderer/Text.h"
#include <string>
#include <format>
#include "ECS/Registry.h"
#include "ECS/View.h"
#include "ECS/Components/BroadCollider.h"
#include "Core/Profiler.h"
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

using namespace Mupfel;

void Mupfel::DebugLayer::OnInit()
{
}

void Mupfel::DebugLayer::OnUpdate(double timestep)
{
}

void Mupfel::DebugLayer::OnRender()
{

	DrawDebugInfo();
}

static float slider_val = 0.0f;

void Mupfel::DebugLayer::DrawDebugInfo()
{
	uint32_t current_entities = Application::GetCurrentRegistry().GetCurrentEntities();
	/* Get the time of the last frame. */
	float last_frame_time = Application::GetLastFrameTime();
	float fps = 1.0f / last_frame_time;

	int screen_height = Application::GetCurrentRenderHeight();
	int screen_width = Application::GetCurrentRenderWidth();

	std::string text1 = std::vformat("FPS: {:.1f}", std::make_format_args(fps));
	std::string text2 = std::vformat("Entities(GLOBAL): {}", std::make_format_args(current_entities));
	Text::RaylibDrawText(text1.c_str(), 10, 20);
	Text::RaylibDrawText(text2.c_str(), 10, 40);


	/* Print the Profiling Samples */
	const std::vector<ProfilingSample>& samples = Profiler::GetCurrentSamples();

	std::vector<ProfilingSample> local(samples.begin(), samples.end());

#if 1
	if (!local.empty())
	{
		// Sortiere stabil nach Startzeit (aufsteigend)
		std::stable_sort(local.begin(), local.end(),
			[](auto const& a, auto const& b) {
				return a.id < b.id;
			});

		std::string t;
		uint32_t offset = 150;
		for (const auto& s : local)
		{
			std::string indent(s.depth * 2, ' ');
			double elapsed_ms = (s.end_time - s.start_time) * 1000.0f;
			t = std::vformat("{}{}: {:.0f}ms", std::make_format_args(indent, s.name, elapsed_ms));
			Text::RaylibDrawText(t.c_str(), 10, offset);
			offset += 20;
		}
	}
#endif
}
