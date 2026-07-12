#include "EditorLayer.h"
#include "Core/Application.h"
#include "ECS/Registry.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"
#include "ECS/Components/Collider.h"

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
}

void EditorLayer::OnRender()
{
}
