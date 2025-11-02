#include "HelloWorldLayer.h"
#include "Renderer/Text.h"
#include "Renderer/Circle.h"
#include "Renderer/Rectangle.h"
#include "Renderer/TextureManager.h"
#include "Core/Application.h"
#include "Core/Profiler.h"
#include <iostream>
#include "ECS/View.h"
#include <format>
#include <string>
#include <algorithm>

#include "ECS/Components/Transform.h"
#include "ECS/Components/Velocity.h"

#include "Renderer/Renderer.h"

using namespace Mupfel;

static Entity *cursor = nullptr;

static const std::string ball_texture_path = "Resources/simple_ball.png";

static uint64_t entities_per_frame = 1;

static std::vector<Entity> current_entities;

void HelloWorldLayer::OnInit()
{
	Registry& reg = Mupfel::Application::GetCurrentRegistry();
	/* Create an Entity for the Cursor */
	cursor = new Entity(reg.CreateEntity());

	/* Add the Kinematic component to it */
	reg.AddComponent<Transform>(*cursor, {});


	int screen_height = Application::GetCurrentRenderHeight();
	int screen_width = Application::GetCurrentRenderWidth();

	for (uint32_t i = 0; i < 10; i++)
	{
		Mupfel::Entity ent = reg.CreateEntity();

		current_entities.push_back(ent);

		float pos_x = (float)Application::GetRandomNumber(1, screen_width - 1);
		float pos_y = (float)Application::GetRandomNumber(1, screen_height - 1);

		float ang_vel = (float)Application::GetRandomNumber(1, 1000) / 200 + 1.0f;

		/* Add velocity component to the entity */
		float vel_x = (float)Application::GetRandomNumber(50, 200);
		float vel_y = (float)Application::GetRandomNumber(50, 200);
		reg.AddComponent<Transform>(ent, { {pos_x, pos_y}, 32.0f, 32.0f, ang_vel });
		reg.AddComponent<Velocity>(ent, { vel_x, vel_y });
		reg.AddComponent<BroadCollider>(ent, { 15, 15 });
		reg.AddComponent<SpatialInfo>(ent, {});
		reg.AddComponent<TextureComponent>(ent, TextureManager::LoadTextureFromFile(ball_texture_path));
	}

}

void HelloWorldLayer::OnUpdate(double timestep)
{
	
	ProcessEvents();
	
}

void HelloWorldLayer::OnRender()
{
}


void HelloWorldLayer::ProcessEvents()
{

	auto& evt_system = Mupfel::Application::GetCurrentEventSystem();
	Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();

	int screen_height = Application::GetCurrentRenderHeight();
	int screen_width = Application::GetCurrentRenderWidth();

	/*
		Retrieve all the UserInputEvents that were issued the last frame.
	*/
	for (const auto& evt : evt_system.GetEvents<Mupfel::UserInputEvent>())
	{
		/* If Left-Mouseclick is pressed, iterate over the entities */
		if (evt.input == Mupfel::UserInput::LEFT_MOUSE_CLICK)
		{
			if (current_entities.size() > 0)
			{
				Entity ent = current_entities.back();
				current_entities.pop_back();
				reg.RemoveComponent<Velocity>(ent);
			}
		}

		if (evt.input == Mupfel::UserInput::WINDOW_FULLSCREEN)
		{
		}

		/* If Right-Mouseclick is pressed, create new entites */
		if (evt.input == Mupfel::UserInput::RIGHT_MOUSE_CLICK)
		{
			for (uint32_t i = 0; i < entities_per_frame; i++)
			{
				Mupfel::Entity ent = reg.CreateEntity();

				current_entities.push_back(ent);

				float pos_x = (float)Application::GetRandomNumber(300, 2300);
				float pos_y = (float)Application::GetRandomNumber(50, 1400);

				float ang_vel = (float)Application::GetRandomNumber(1, 1000) / 200 + 1.0f;

				/* Add velocity component to the entity */
				float vel_x = (float)Application::GetRandomNumber(50, 200);
				float vel_y = (float)Application::GetRandomNumber(50, 200);
				reg.AddComponent<Transform>(ent, { {pos_x, pos_y}, 32.0f, 32.0f, ang_vel });
				reg.AddComponent<Velocity>(ent, { vel_x, vel_y });
				reg.AddComponent<BroadCollider>(ent, { 15, 15 });
				reg.AddComponent<SpatialInfo>(ent, {});
				reg.AddComponent<TextureComponent>(ent, TextureManager::LoadTextureFromFile(ball_texture_path));
			}
		}


		/* If the Cursor Position changed, we update our entity */
		if (evt.input == Mupfel::UserInput::CURSOR_POS_CHANGED)
		{
			auto kinematic = reg.GetComponent<Transform>(*cursor);
			kinematic.pos.x = Application::GetCurrentInputManager().GetCurrentCursorX();
			kinematic.pos.y = Application::GetCurrentInputManager().GetCurrentCursorY();
			reg.SetComponent(*cursor, kinematic);
		}

		if (evt.input == Mupfel::UserInput::MOVE_FORWARD)
		{
			entities_per_frame = std::clamp<uint64_t>((entities_per_frame + 50), 1, 10000000);
		}

	}
}
