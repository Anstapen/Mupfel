#include "EditorLayer.h"
#include "raygui.h"
#include "Core/Application.h"
#include "ECS/Registry.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Velocity.h"
#include "ECS/Components/SpatialInfo.h"
#include "Renderer/Texture.h"


static Mupfel::Entity* cursor = nullptr;

static Mupfel::Entity* preview = nullptr;

static float scale = 32.0f;
static bool angular_velocity_wanted = false;
static float angular_velocity = 0;
static bool spatial_info_wanted = false;
static int entity_count = 1;
static bool entity_count_edit = false;

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
	reg.AddComponent<Mupfel::TextureComponent>(*preview, {});
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
	
	Vector2 anchor01 = { screen_width - 300, anchor_y };

    // raygui: controls drawing
            //----------------------------------------------------------------------------------
    GuiGroupBox(Rectangle(anchor01.x, anchor01.y + 40, 250, 500), "Create Entity");
	GuiCheckBox(Rectangle(anchor01.x + 50, anchor01.y + 110, 24, 24), "Angular Velocity", &angular_velocity_wanted);
	GuiSlider(Rectangle(anchor01.x + 50, anchor01.y + 140, 120, 24), "Vel:", NULL, &angular_velocity, 0, PI * 2 * 10);
    GuiSlider(Rectangle(anchor01.x + 50, anchor01.y + 170, 120, 24), "Scale", NULL, &scale, 1, 100);
    GuiCheckBox(Rectangle(anchor01.x + 50, anchor01.y + 230, 24, 24), "SpatialInfo", &spatial_info_wanted);
	if (GuiValueBox(Rectangle(anchor01.x + 50, anchor01.y + 260, 120, 24), "No.", & entity_count, 1, 100000, entity_count_edit)) entity_count_edit = !entity_count_edit;
	GuiGroupBox(Rectangle(anchor01.x + 85, anchor01.y + 400, 100, 100), "Preview");
    //----------------------------------------------------------------------------------

	Mupfel::Transform t;
	t.pos_x = anchor01.x +135;
	t.pos_y = anchor01.y + 450;
	t.scale_x = scale;
	t.scale_y = scale;

	/* Reposition the entity */
	reg.SetComponent<Mupfel::Transform>(*preview, t);

}

static float initial_x = 0.0f;
static float initial_y = 0.0f;
static float velocity_x = 0.0f;
static float velocity_y = 0.0f;

static Mupfel::Entity currently_created_entity = 0;

void EditorLayer::ProcessEvents()
{
	auto& evt_system = Mupfel::Application::GetCurrentEventSystem();
	Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();

	int screen_height = Mupfel::Application::GetCurrentRenderHeight();
	int screen_width = Mupfel::Application::GetCurrentRenderWidth();

	/*
		Retrieve all the UserInputEvents that were issued the last frame.
	*/
	for (const auto& evt : evt_system.GetEvents<Mupfel::UserInputEvent>())
	{
		/* If Left-Mouseclick is pressed, iterate over the entities */
		if (evt.input == Mupfel::UserInput::RIGHT_MOUSE_CLICK)
		{
		}

	}
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		initial_x = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorX();
		initial_y = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorY();

		currently_created_entity = reg.CreateEntity();

		Mupfel::Transform t;
		t.pos_x = initial_x;
		t.pos_y = initial_y;
		t.scale_x = scale;
		t.scale_y = scale;
		reg.AddComponent<Mupfel::Transform>(currently_created_entity, t);
		reg.AddComponent<Mupfel::TextureComponent>(currently_created_entity, {});
	}

	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		float current_x = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorX();
		float current_y = Mupfel::Application::GetCurrentInputManager().GetCurrentCursorY();

		DrawLine(current_x, current_y, initial_x, initial_y, RED);
		velocity_x = (initial_x - current_x) * 2;
		velocity_y = (initial_y - current_y) * 2;
	}

	if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
	{
		Mupfel::Velocity v;
		v.x = velocity_x;
		v.y = velocity_y;
		v.angular = angular_velocity;
		reg.AddComponent<Mupfel::Velocity>(currently_created_entity, v);
		reg.AddComponent<Mupfel::SpatialInfo>(currently_created_entity, {});
	}

}
