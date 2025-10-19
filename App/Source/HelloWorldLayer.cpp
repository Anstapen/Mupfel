#include "HelloWorldLayer.h"
#include "Renderer/Text.h"
#include "Renderer/Circle.h"
#include "Renderer/Rectangle.h"
#include "Renderer/TextureManager.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/BroadCollider.h"
#include "ECS/Components/SpatialInfo.h"
#include "Core/Profiler.h"
#include <iostream>
#include "ECS/View.h"
#include <format>
#include <string>
#include <algorithm>

using namespace Mupfel;

struct Velocity {
	float x, y;
};

struct TextureComponent {
	SafeTexturePointer texture;
};

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
	ProfilingSample on_update("HelloWorld OnUpdate");
	
	ProcessEvents();
	
	UpdateEntityPositions();

	CleanUpEntities();
	
	MarkDirtyEntities();	
	
}

void HelloWorldLayer::OnRender()
{
#if 0
	{
		ProfilingSample prof("HelloWorld Render");
		/* Render all entities */
		auto view = Application::GetCurrentRegistry().view<TextureComponent, Transform>();

		for (auto [entity, texture, transform] : view)
		{
			uint32_t render_pos_x = transform.pos.x - (texture.texture->width / 2);
			uint32_t render_pos_y = transform.pos.y - (texture.texture->height / 2);
			Texture::RaylibDrawTexture(*texture.texture.get(), render_pos_x, render_pos_y);
		}
	}
#endif
}


void HelloWorldLayer::ProcessEvents()
{
	ProfilingSample prof("ProcessEvents");

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
			entities_per_frame = std::clamp<uint64_t>((entities_per_frame + 100), 1, 10000000);
		}
	}
}

void HelloWorldLayer::UpdateEntityPositions()
{
	ProfilingSample prof("UpdateEntityPositions");

	float delta_time = Application::GetLastFrameTime();

	Registry& reg = Application::GetCurrentRegistry();

	int screen_height = Application::GetCurrentRenderHeight();
	int screen_width = Application::GetCurrentRenderWidth();

	entity_transform_container.clear();
	entity_garbage.clear();

	ComponentArray<BroadCollider>& broad_collider_array = reg.GetComponentArray<BroadCollider>();

	reg.ParallelForEach<Transform, Velocity>([this, delta_time, &reg, screen_width, screen_height, &broad_collider_array](Entity e, Transform& t, Velocity& v)
		{

			if (v.x == 0.0f || v.y == 0.0f)
			{
				/* This Entity currently has a Velocity of 0 */
				return false;
			}

			t.pos.x += v.x * delta_time;
			t.pos.y += v.y * delta_time;


			if (t.pos.x > screen_width || t.pos.x < 0.0f || t.pos.y > screen_height || t.pos.y < 0.0f)
			{
				std::scoped_lock lock(garbage_mutex);
				entity_garbage.push_back(e);
			}

			/* Update the BroadCollider, if there is one */
			if (broad_collider_array.Has(e))
			{
				auto& broad_collider = broad_collider_array.Get(e);
				broad_collider.max.x = t.pos.x + broad_collider.offset.x;
				broad_collider.max.y = t.pos.y + broad_collider.offset.y;
				broad_collider.min.x = t.pos.x - broad_collider.offset.x;
				broad_collider.min.y = t.pos.y - broad_collider.offset.y;

				/* Clamp the top-left values */
				broad_collider.min.x = broad_collider.min.x < 0.0f ? 0.0f : broad_collider.min.x;
				broad_collider.min.y = broad_collider.min.y < 0.0f ? 0.0f : broad_collider.min.y;
			}

			return true;

		},
		entity_transform_container
	);
}

void HelloWorldLayer::CleanUpEntities()
{
	ProfilingSample prof("CleanUpEntities");

	Registry& reg = Application::GetCurrentRegistry();
	for (auto e : entity_garbage)
	{
		reg.DestroyEntity(e);
	}
}

void HelloWorldLayer::MarkDirtyEntities()
{
	ProfilingSample prof("MarkDirtyEntities");

	Registry& reg = Application::GetCurrentRegistry();
	ComponentArray<Transform>& transform_array = reg.GetComponentArray<Transform>();

	for (auto e : entity_transform_container)
	{
		transform_array.MarkDirty(e);
	}
}
