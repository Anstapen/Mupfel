#include "HelloWorldLayer.h"
#include "Renderer/Text.h"
#include "Renderer/Circle.h"
#include "Renderer/Rectangle.h"
#include "Renderer/TextureManager.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/BroadCollider.h"
#include "ECS/Components/SpatialInfo.h"
#include "ECS/Components/Velocity.h"
#include "Core/Profiler.h"
#include <iostream>
#include "ECS/View.h"
#include <format>
#include <string>
#include <algorithm>

#include "Renderer/Renderer.h"

using namespace Mupfel;

static Entity *cursor = nullptr;

static const std::string ball_texture_path = "Resources/simple_ball.png";

static uint64_t entities_per_frame = 1;

void HelloWorldLayer::OnInit()
{
	Registry& reg = Mupfel::Application::GetCurrentRegistry();
	/* Create an Entity for the Cursor */
	cursor = new Entity(reg.CreateEntity());

	/* Add the Position component to it */
	reg.AddComponent<Transform>(*cursor, { 0 ,0 });

}

void HelloWorldLayer::OnUpdate(float timestep)
{

	Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();

	int screen_height = Application::GetCurrentRenderHeight();
	int screen_width = Application::GetCurrentRenderWidth();

	for (uint32_t i = 0; i < entities_per_frame; i++)
	{
		Mupfel::Entity ent = reg.CreateEntity();

		float pos_x = (float)Application::GetRandomNumber(1, screen_width - 1);
		float pos_y = (float)Application::GetRandomNumber(1, screen_height - 1);

		/* Add velocity component to the entity */
		float vel_x = (float)Application::GetRandomNumber(50, 200);
		float vel_y = (float)Application::GetRandomNumber(50, 200);
		reg.AddComponent<Velocity>(ent, { vel_x, vel_y });
		reg.AddComponent<Transform>(ent, { pos_x, pos_y });
		reg.AddComponent<BroadCollider>(ent, { 15, 15 });
		reg.AddComponent<SpatialInfo>(ent, {});
		reg.AddComponent<TextureComponent>(ent, TextureManager::LoadTextureFromFile(ball_texture_path));
	}
	
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
		}

		if (evt.input == Mupfel::UserInput::WINDOW_FULLSCREEN)
		{
		}

		/* If Right-Mouseclick is pressed, create new entites */
		if (evt.input == Mupfel::UserInput::RIGHT_MOUSE_CLICK)
		{
		}

		ComponentArray<Transform>& transform_array = reg.GetComponentArray<Transform>();

		/* If the Cursor Position changed, we update our entity */
		if (evt.input == Mupfel::UserInput::CURSOR_POS_CHANGED)
		{
			auto& transform = transform_array.Get(*cursor);
			transform.pos.x = Application::GetCurrentInputManager().GetCurrentCursorX();
			transform.pos.y = Application::GetCurrentInputManager().GetCurrentCursorY();
		}

		if (evt.input == Mupfel::UserInput::MOVE_FORWARD)
		{
			entities_per_frame = std::clamp<uint64_t>((entities_per_frame + 10), 1, 10000000);
		}

		if (evt.input == Mupfel::UserInput::MOVE_BACKKWARDS)
		{
			Mupfel::Renderer::ToggleMode();
		}
	}
}
