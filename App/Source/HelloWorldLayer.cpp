#include "HelloWorldLayer.h"
#include "Renderer/Text.h"
#include "Renderer/Circle.h"
#include "Renderer/Rectangle.h"
#include "Core/Application.h"
#include <iostream>
#include "ECS/View.h"
#include <format>

using namespace Mupfel;

struct Velocity {
	float x, y;
};

struct Position {
	float x, y;
};

struct CollisionCircle {
	float r;
};

struct RenderableCircleOutline {
	float radius;
	uint8_t col_r;
	uint8_t col_g;
	uint8_t col_b;
	uint8_t alpha;
};

static_assert(sizeof(RenderableCircleOutline) == 8);

static Entity *cursor = nullptr;

void HelloWorldLayer::OnInit()
{
	Registry& reg = Mupfel::Application::GetCurrentRegistry();
	/* Create an Entity for the Cursor */
	cursor = new Entity(reg.CreateEntity());

	/* Add the Position component to it */
	reg.AddComponent<Position>(*cursor, { 0 ,0 });
}

void HelloWorldLayer::OnUpdate(float timestep)
{

	auto &evt_system = Mupfel::Application::GetCurrentEventSystem();
	Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();
	/*
		Retrieve all the UserInputEvents that were issued the last frame.
	*/
	for (const auto& evt : evt_system.GetEvents<Mupfel::UserInputEvent>())
	{
		/* If Left-Mouseclick is pressed, iterate over the entities */
		if (evt.input == Mupfel::UserInput::LEFT_MOUSE_CLICK)
		{
		}

		/* If Right-Mouseclick is pressed, create new entites */
		if (evt.input == Mupfel::UserInput::RIGHT_MOUSE_CLICK)
		{
			float pos_x = (float)Application::GetCurrentInputManager().GetCurrentCursorX();
			float pos_y = (float)Application::GetCurrentInputManager().GetCurrentCursorY();

			for (uint32_t i = 0; i < 1000; i++)
			{
				Mupfel::Entity ent = reg.CreateEntity();

				/* Add velocity component to the entity */
				float vel_x = (float)Application::GetRandomNumber(50, 200);
				float vel_y = (float)Application::GetRandomNumber(50, 200);
				reg.AddComponent<Velocity>(ent, { vel_x, vel_y });
				reg.AddComponent<Position>(ent, { pos_x, pos_y });
				reg.AddComponent<CollisionCircle>(ent, { 10.0f });
				reg.AddComponent<RenderableCircleOutline>(ent, { 10.0f, 0, 228, 48, 255 });
			}
		}

		/* If the Cursor Position changed, we update our entity */
		if (evt.input == Mupfel::UserInput::CURSOR_POS_CHANGED)
		{
			auto& pos = reg.GetComponent<Position>(*cursor);
			pos.x = Application::GetCurrentInputManager().GetCurrentCursorX();
			pos.y = Application::GetCurrentInputManager().GetCurrentCursorY();
		}

		/* If the Cursor Position changed, we update our entity */
		if (evt.input == Mupfel::UserInput::MOVE_FORWARD)
		{
			auto position_view = reg.view<Position>();

			for (auto [entity, position] : position_view)
			{
				if (reg.HasComponent<RenderableCircleOutline>(entity)) {
					reg.RemoveComponent<RenderableCircleOutline>(entity);
				}
			}
		}
		{
			auto& pos = reg.GetComponent<Position>(*cursor);
			pos.x = Application::GetCurrentInputManager().GetCurrentCursorX();
			pos.y = Application::GetCurrentInputManager().GetCurrentCursorY();
		}
	}
	auto position_view = reg.view<Velocity, Position>();

	int screen_height = Application::GetCurrentRenderHeight()-50;
	int screen_width = Application::GetCurrentRenderWidth()-50;

	static std::vector<Entity> entity_garbage;

	entity_garbage.clear();

	float delta_time = Application::GetLastFrameTime();

	/* Update Position on the screen, remember entities that leave it */
	for (auto [entity, velocity, position] : position_view)
	{
		position.x += velocity.x * delta_time;
		position.y += velocity.y * delta_time;

		if (position.x > screen_width || position.x < 50.0f || position.y > screen_height || position.y < 50.0f)
		{
			entity_garbage.push_back(entity);
		}
	}

	for (auto e : entity_garbage)
	{
		reg.DestroyEntity(e);
	}
}

void HelloWorldLayer::OnRender()
{
	/* Position Entities */
	auto position_view = Application::GetCurrentRegistry().view<Position>();
	uint32_t pos_ent_count = 0;
	
	for (auto [entity, position] : position_view)
	{
		pos_ent_count++;
	}

	/* Velocity Entities */
	auto velocity_view = Application::GetCurrentRegistry().view<Velocity>();
	uint32_t vel_ent_count = 0;

	for (auto [entity, vel] : velocity_view)
	{
		vel_ent_count++;
	}

	/* CollisionCircle Entities */
	auto coll_view = Application::GetCurrentRegistry().view<CollisionCircle>();
	uint32_t coll_ent_count = 0;

	for (auto [entity, coll] : coll_view)
	{
		coll_ent_count++;
	}

	/* RenderableCircle Entities */
	auto rend_view = Application::GetCurrentRegistry().view<RenderableCircleOutline>();
	uint32_t rend_ent_count = 0;

	for (auto [entity, rend] : rend_view)
	{
		rend_ent_count++;
	}
	uint32_t current_entities = Application::GetCurrentRegistry().GetCurrentEntities();
	/* Get the time of the last frame. */
	float last_frame_time = Application::GetLastFrameTime();
	float fps = 1.0f / last_frame_time;

	int screen_height = Application::GetCurrentRenderHeight();
	int screen_width = Application::GetCurrentRenderWidth();

	size_t free_list = Application::GetCurrentRegistry().GetFreeSIZE();

	uint64_t events_last_frame = Application::GetCurrentEventSystem().GetLastEventCount();
	std::string text1 = std::vformat("Frame Time: {:.3f}, FPS: {:.1f}, Entities(GLOBAL): {} Events: {}", std::make_format_args(last_frame_time, fps, current_entities, events_last_frame));
	std::string text2 = std::vformat("Screen Height: {}, Screen Width: {}", std::make_format_args(screen_height, screen_width));
	std::string pos_ents = std::vformat("Entities(POS): {}", std::make_format_args(pos_ent_count));
	std::string vel_ents = std::vformat("Entities(VEL): {}", std::make_format_args(vel_ent_count));
	std::string coll_ents = std::vformat("Entities(COLL): {}", std::make_format_args(coll_ent_count));
	std::string rend_ents = std::vformat("Entities(REND): {}", std::make_format_args(rend_ent_count));
	std::string free_list_size = std::vformat("Free List: {}", std::make_format_args(free_list));
	Text::RaylibDrawText(text1.c_str(), 50, 50);
	Text::RaylibDrawText(text2.c_str(), 50, 100);
	Text::RaylibDrawText(pos_ents.c_str(), 50, 120);
	Text::RaylibDrawText(vel_ents.c_str(), 50, 140);
	Text::RaylibDrawText(coll_ents.c_str(), 50, 160);
	Text::RaylibDrawText(rend_ents.c_str(), 50, 180);
	Text::RaylibDrawText(free_list_size.c_str(), 50, 200);

	auto& pos = Application::GetCurrentRegistry().GetComponent<Position>(*cursor);
	/* Draw a Cirle around the Cursor */
	Circle::RayLibDrawCircleLines(pos.x, pos.y, 50.0f);
	Rectangle::RaylibDrawRect(50, 50, screen_width - 100, screen_height - 100, 255, 109, 194, 255);
	

	/* Render all entities */
	auto view = Application::GetCurrentRegistry().view<RenderableCircleOutline, Position>();

	for (auto [entity, circle, position] : view)
	{
		Circle::RayLibDrawCircleLines(position.x, position.y, circle.radius, circle.col_r, circle.col_g, circle.col_b, circle.alpha);
	}
}
