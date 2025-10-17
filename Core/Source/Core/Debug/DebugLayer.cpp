#include "DebugLayer.h"
#include "Core/Application.h"
#include "Renderer/Rectangle.h"
#include "Renderer/Text.h"
#include <string>
#include <format>
#include "ECS/Registry.h"
#include "ECS/View.h"
#include "ECS/Components/BroadCollider.h"

using namespace Mupfel;

void Mupfel::DebugLayer::OnInit()
{
}

void Mupfel::DebugLayer::OnUpdate(float timestep)
{
}

void Mupfel::DebugLayer::OnRender()
{
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

	/* Draw the BroadColliders of each entity */
	Registry& reg = Application::GetCurrentRegistry();

#if 1
	auto position_view = reg.view<BroadCollider>();
	for (auto [entity, broad] : position_view)
	{
		Rectangle::RaylibDrawRect(broad.min.x, broad.min.y, broad.max.x - broad.min.x, broad.max.y - broad.min.y, 102, 191, 255, 255);
	}
#endif
}
