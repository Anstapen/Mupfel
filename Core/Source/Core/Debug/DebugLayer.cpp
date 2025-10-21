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

void Mupfel::DebugLayer::OnUpdate(float timestep)
{
}

void Mupfel::DebugLayer::OnRender()
{
#if 0
	/* Lets render the Collision Grid */
	if (!Application::isDebugModeEnabled())
	{
		return;
	}
	uint32_t screen_width = Application::GetCurrentRenderWidth();
	uint32_t screen_height = Application::GetCurrentRenderHeight();

	static uint32_t num_rows = Application::Get().collision_system.num_cells_y;
	static uint32_t num_columns = Application::Get().collision_system.num_cells_x;
	static uint32_t cell_size = 1 << Application::Get().collision_system.cell_size_pow;

	uint32_t pos_x = 0;
	uint32_t pos_y = 0;
	for (uint32_t y = 0; y < num_rows; y++)
	{
		if (pos_y > screen_height)
		{
			break;
		}

		for (uint32_t x = 0; x < num_columns; x++)
		{
			if (pos_x > screen_width)
			{
				break;
			}

			uint32_t cell_index = CollisionSystem::WorldtoCell({ pos_x, pos_y });

			if (Application::Get().collision_system.collision_grid.cells[cell_index].count == 0)
			{
				Rectangle::RaylibDrawRect(pos_x, pos_y, cell_size, cell_size, 230, 41, 55, 255);
				uint32_t cell_count = Application::Get().collision_system.collision_grid.cells[cell_index].count;
				std::string text = std::vformat("{}", std::make_format_args(cell_count));
				Text::RaylibDrawText(text.c_str(), pos_x, pos_y);
			}
			else
			{
				Rectangle::RaylibDrawRect(pos_x, pos_y, cell_size, cell_size, 0, 228, 48, 255);
				uint32_t cell_count = Application::Get().collision_system.collision_grid.cells[cell_index].count;
				std::string text = std::vformat("{}", std::make_format_args(cell_count));
				Text::RaylibDrawText(text.c_str(), pos_x, pos_y);
			}
			

			pos_x += cell_size;
		}
		pos_x = 0;
		
		pos_y += cell_size;
	}
#endif

	DrawDebugInfo();
}

static float slider_val = 0.0f;

void Mupfel::DebugLayer::DrawDebugInfo()
{
	uint32_t current_entities = Application::GetCurrentRegistry().GetCurrentEntities() / 1000;
	/* Get the time of the last frame. */
	float last_frame_time = Application::GetLastFrameTime();
	float fps = 1.0f / last_frame_time;

	int screen_height = Application::GetCurrentRenderHeight();
	int screen_width = Application::GetCurrentRenderWidth();

	std::string text1 = std::vformat("FPS: {:.1f}", std::make_format_args(fps));
	std::string text2 = std::vformat("Entities(GLOBAL): {}k", std::make_format_args(current_entities));
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
