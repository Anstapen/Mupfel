#include "EditorLayer.h"
#include "raygui.h"
#include "Core/Application.h"
#include "ECS/Registry.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"
#include "ECS/Components/Collider.h"
#include "Renderer/Texture.h"
#include "Renderer/Circle.h"
#include "Renderer/Rectangle.h"

/* Entity load store */
#include "FS/EntityFileManager.h"

#include <cmath>


static Mupfel::Entity* cursor = nullptr;

static Mupfel::Entity* preview = nullptr;

static bool angular_velocity_wanted = false;
static float angular_velocity = 0;
static bool collider_wanted = false;
static float collider_size = 1.0f;
static bool velocity_wanted = false;

void EditorLayer::OnInit()
{
	Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();
	/* Create an Entity for the Cursor */
	cursor = new Mupfel::Entity(reg.CreateEntity());

	preview = new Mupfel::Entity(reg.CreateEntity());

	/* Add the Transform component to it */
	Mupfel::Transform t;
	t.pos_x = 0.0f;
	t.pos_y = 0.0f;
	t.scale_x = 32.0f;
	t.scale_y = 32.0f;
	t.rotation = 0.0f;
	reg.AddComponent<Mupfel::Transform>(*cursor, t);
	reg.AddComponent<Mupfel::Transform>(*preview, t);

}

void EditorLayer::OnUpdate(double timestep)
{
	/* Update the Entity Properties */
	ProcessEvents();
}

void EditorLayer::OnRender()
{
	uint32_t screen_height = Mupfel::Application::GetCurrentRenderHeight();
	uint32_t screen_width = Mupfel::Application::GetCurrentRenderWidth();
	Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();

	uint32_t anchor_x = static_cast<uint32_t>(static_cast<float>(screen_width) * 2 / 3);
	uint32_t anchor_y = static_cast<uint32_t>(static_cast<float>(screen_height) * 1 / 10);
	
	Vector2 anchor = { screen_width - 300, anchor_y };

    /* Draw common GUI elements */
    GuiGroupBox(Rectangle(anchor.x, anchor.y + 40, 250, 600), "Create Entity");
	int testval = 0;
	

	GuiCheckBox(Rectangle(anchor.x + 50, anchor.y + 140, 24, 24), "Velocity", &velocity_wanted);

	GuiCheckBox(Rectangle(anchor.x + 50, anchor.y + 170, 24, 24), "Angular Velocity", &angular_velocity_wanted);
	GuiSlider(Rectangle(anchor.x + 50, anchor.y + 200, 120, 24), "Vel:", NULL, &angular_velocity, 0, PI * 2 * 10);
	GuiSlider(Rectangle(anchor.x + 50, anchor.y + 230, 120, 24), "Scale", NULL, &current_scale, 1, 100);
	GuiCheckBox(Rectangle(anchor.x + 50, anchor.y + 260, 24, 24), "Collider", &collider_wanted);
	
	if (GuiButton(Rectangle(anchor.x + 125 - 25, anchor.y + 550, 50, 50), "Load Entities")) {
		/* Try to load entities */
		Mupfel::EntityFileManager fm;

		fm.Load("Data/entities.json");
	}

	if (GuiDropdownBox(Rectangle(anchor.x + 50, anchor.y + 60, 60, 24), "Circle;AABB", &DropdownBox000Active, DropdownBox000EditMode))
	{
		DropdownBox000EditMode = !DropdownBox000EditMode;
	}

	Vector2 preview_anchor = { anchor.x + 125 - 50, anchor.y + 400 };
	GuiGroupBox(Rectangle(preview_anchor.x, preview_anchor.y, 100, 100), "Preview:");
	DrawPreview(preview_anchor);
    
	Vector2 entity_creator_anchor = { anchor.x, anchor.y + 290 };
	switch (DropdownBox000Active)
	{
		case 0:
			DrawCircleCreator(entity_creator_anchor);
			break;
		case 1:
			DrawAABBCreator(entity_creator_anchor);
			break;
		default:
			break;
	}

	DrawRects();

}

void EditorLayer::ProcessEvents()
{

	if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
	{
		CreateNewEntity();
	}

	if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		initial_velocity_x = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorX();
		initial_velocity_y = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorY();
	}

	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && velocity_wanted)
	{
		float current_x = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorX();
		float current_y = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorY();

		DrawLine(current_x, current_y, initial_velocity_x, initial_velocity_y, RED);
		current_velocity_x = (initial_velocity_x - current_x) * 2;
		current_velocity_y = (initial_velocity_y - current_y) * 2;
	}
	
}

void EditorLayer::CreateNewEntity()
{
	Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();

	Mupfel::Entity currently_created_entity = reg.CreateEntity();

	Mupfel::Transform t;
	t.pos_x = current_pos_x;
	t.pos_y = current_pos_y;
	t.scale_x = current_scale;
	t.scale_y = current_scale;
	reg.AddComponent<Mupfel::Transform>(currently_created_entity, t);

	if(velocity_wanted)
	{
		Mupfel::Movement movement;
		movement.velocity_x = current_velocity_x;
		movement.velocity_y = current_velocity_y;
		movement.angular_velocity = angular_velocity;
		reg.AddComponent<Mupfel::Movement>(currently_created_entity, movement);
	}

	/* For Circles, we have a Texture */
	if (DropdownBox000Active == static_cast<int>(ShapeType::Circle) - 1)
	{
		reg.AddComponent<Mupfel::TextureComponent>(currently_created_entity, {});
	}

	if (!collider_wanted)
	{
		return;
	}

	Mupfel::Collider c;
	switch (DropdownBox000Active)
	{
	case 0: // Circle
		c.SetCircle(collider_size);
		reg.AddComponent<Mupfel::Collider>(currently_created_entity, c);
		break;
	case 1: // AABB
		c.SetAABB(collider_size_x, collider_size_y);
		reg.AddComponent<Mupfel::Collider>(currently_created_entity, c);
		break;
	default:
		break;
	}
}

void EditorLayer::DrawCircleCreator(Vector2 anchor)
{
	GuiSlider(Rectangle(anchor.x + 50, anchor.y, 120, 24), "Size", NULL, &collider_size, 16, 200);
	collider_size_x = collider_size;
	collider_size_y = collider_size;
	Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();
	current_pos_x = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorX();
	current_pos_y = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorY();

}
void EditorLayer::DrawAABBCreator(Vector2 anchor)
{
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		initial_pos_x = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorX();
		initial_pos_y = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorY();
	}

	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		float current_size_x = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorX() - initial_pos_x;
		float current_size_y = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorY() - initial_pos_y;

		current_pos_x = initial_pos_x + current_size_x / 2;
		current_pos_y = initial_pos_y + current_size_y / 2;

		RaylibDrawRectFilled(current_pos_x - current_size_x / 2, current_pos_y - current_size_y / 2, current_size_x, current_size_y, 102, 191, 255, 255);

		collider_size_x = std::abs(current_size_x);
		collider_size_y = std::abs(current_size_y);
	}
}

void EditorLayer::DrawPreview(Vector2 anchor)
{
	Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();

	Mupfel::Transform t;
	t.pos_x = anchor.x + 50;
	t.pos_y = anchor.y + 50;
	t.scale_x = current_scale;
	t.scale_y = current_scale;

	/* Reposition the entity */
	reg.SetComponent<Mupfel::Transform>(*preview, t);

	/* Reset the texture component */
	if(reg.HasComponent<Mupfel::TextureComponent>(*preview))
	{
		reg.RemoveComponent<Mupfel::TextureComponent>(*preview);
	}

	if (DropdownBox000Active == static_cast<int>(ShapeType::Circle) - 1)
	{
		reg.AddComponent<Mupfel::TextureComponent>(*preview, {});
	}

	if (DropdownBox000Active == static_cast<int>(ShapeType::AABB) - 1)
	{
		RaylibDrawRectFilled(t.pos_x - t.scale_x / 2, t.pos_y - t.scale_y / 2, t.scale_x, t.scale_y, 102, 191, 255, 255);
	}

	/* Draw the collider if enabled */
	if (collider_wanted)
	{
		switch (DropdownBox000Active)
		{
		case 0:
			Mupfel::Circle::RayLibDrawCircleLines(t.pos_x, t.pos_y, collider_size, 102, 191, 255, 255);
			break;
		case 1:
			break;
		default:
			break;
		}
	}
}

void EditorLayer::DrawRects()
{
	Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();

	auto rect_view = reg.view<Mupfel::Transform, Mupfel::Collider>();

	for (auto [entity, t, collider] : rect_view)
	{
		if(collider.info.type != ShapeType::Circle)
		{
			RaylibDrawRect(t.pos_x - collider.GetBoundingBoxX() / 2, t.pos_y - collider.GetBoundingBoxY() / 2, collider.GetBoundingBoxX(), collider.GetBoundingBoxY(), 102, 191, 255, 255);
		}
	}
}
