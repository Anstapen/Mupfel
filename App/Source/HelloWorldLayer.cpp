#include "HelloWorldLayer.h"
#include "Renderer/Text.h"
#include "Renderer/Circle.h"
#include "Renderer/Rectangle.h"
#include "Renderer/TextureManager.h"
#include "Core/Application.h"
#include <iostream>
#include "ECS/View.h"
#include <format>
#include <string>
#include <algorithm>

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

struct TextureComponent {
	SafeTexturePointer texture;
};

static_assert(sizeof(RenderableCircleOutline) == 8);

static Entity *cursor = nullptr;

static const std::string ball_texture_path = "Resources/simple_ball.png";

static uint64_t entities_per_frame = 10;

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

		if (evt.input == Mupfel::UserInput::WINDOW_FULLSCREEN)
		{

		}

		/* If Right-Mouseclick is pressed, create new entites */
		if (evt.input == Mupfel::UserInput::RIGHT_MOUSE_CLICK)
		{
			float pos_x = (float)Application::GetCurrentInputManager().GetCurrentCursorX();
			float pos_y = (float)Application::GetCurrentInputManager().GetCurrentCursorY();

			for (uint32_t i = 0; i < entities_per_frame; i++)
			{
				Mupfel::Entity ent = reg.CreateEntity();

				/* Add velocity component to the entity */
				float vel_x = (float)Application::GetRandomNumber(50, 200);
				float vel_y = (float)Application::GetRandomNumber(50, 200);
				reg.AddComponent<Velocity>(ent, { vel_x, vel_y });
				reg.AddComponent<Position>(ent, { pos_x, pos_y });
				reg.AddComponent<CollisionCircle>(ent, { 10.0f });
				reg.AddComponent<TextureComponent>(ent, TextureManager::LoadTextureFromFile(ball_texture_path));
			}
		}

		/* If the Cursor Position changed, we update our entity */
		if (evt.input == Mupfel::UserInput::CURSOR_POS_CHANGED)
		{
			auto& pos = reg.GetComponent<Position>(*cursor);
			pos.x = Application::GetCurrentInputManager().GetCurrentCursorX();
			pos.y = Application::GetCurrentInputManager().GetCurrentCursorY();
		}

		if (evt.input == Mupfel::UserInput::MOVE_FORWARD)
		{
			entities_per_frame = std::clamp<uint64_t>((entities_per_frame + 100), 1, 10000000);
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
	
	uint32_t current_entities = Application::GetCurrentRegistry().GetCurrentEntities();
	/* Get the time of the last frame. */
	float last_frame_time = Application::GetLastFrameTime();
	float fps = 1.0f / last_frame_time;

	int screen_height = Application::GetCurrentRenderHeight();
	int screen_width = Application::GetCurrentRenderWidth();

	uint64_t events_last_frame = Application::GetCurrentEventSystem().GetLastEventCount();
	std::string text1 = std::vformat("Frame Time: {:.3f}, FPS: {:.1f}, Entities(GLOBAL): {} Events: {}", std::make_format_args(last_frame_time, fps, current_entities, events_last_frame));
	std::string text2 = std::vformat("Screen Height: {}, Screen Width: {}", std::make_format_args(screen_height, screen_width));
	std::string text4 = std::vformat("Entities per added per Frame: {}", std::make_format_args(entities_per_frame));
	Text::RaylibDrawText(text1.c_str(), 50, 40);
	Text::RaylibDrawText(text2.c_str(), 50, 60);
	Text::RaylibDrawText(text4.c_str(), 50, 80);

	auto& pos = Application::GetCurrentRegistry().GetComponent<Position>(*cursor);
	/* Draw a Cirle around the Cursor */
	Circle::RayLibDrawCircleLines(pos.x, pos.y, 50.0f);
	Rectangle::RaylibDrawRect(50, 50, screen_width - 100, screen_height - 100, 255, 109, 194, 255);
	

	/* Render all entities */
	auto view = Application::GetCurrentRegistry().view<TextureComponent, Position>();

	for (auto [entity, texture, position] : view)
	{
		Texture::RaylibDrawTexture(*texture.texture.get(), position.x, position.y);
	}
}
