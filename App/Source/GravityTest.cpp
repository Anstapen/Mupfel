#include "GravityTest.h"
#include "Imager.h"
#include "SceneSwitches.h"

using namespace Mupfel;

void GravityTest::OnInit()
{
	Entity	  ground = Entities::Create();
	Transform t;
	t.pos_x = -5;
	Entities::AddComponent<Transform>(ground, t);
	Entities::AddComponent<RigidBody>(ground, {});
	Collider c;
	c.half_width = 10;
	c.half_height = 1;
	Entities::AddComponent<Collider>(ground, c);
	Entity side_right = Entities::Create();
	t.pos_x = 4.5;
	t.pos_y = 5;
	Entities::AddComponent<Transform>(side_right, t);
	c.half_width = 0.5;
	c.half_height = 4;
	Entities::AddComponent<Collider>(side_right, c);
	Entities::AddComponent<RigidBody>(side_right, {});
	Entity side_left = Entities::Create();
	t.pos_x = -14.5;
	t.pos_y = 5;
	Entities::AddComponent<Transform>(side_left, t);
	c.half_width = 0.5;
	c.half_height = 4;
	Entities::AddComponent<Collider>(side_left, c);
	Entities::AddComponent<RigidBody>(side_left, {});

	/* Add some balls */
	Imager::Load("Ball", "Images/ball_blue.png");

	float pos_x = -4;

	for (uint32_t i = 0; i < 5; i++)
	{
		Entity	  e = Entities::Create();
		Transform t;
		t.pos_x = pos_x;
		t.pos_y = 5;
		t.pos_z = 0.08f;
		Entities::AddComponent<Transform>(e, t);
		RigidBody b;
		b.type = BodyType::Dynamic;
		Entities::AddComponent<RigidBody>(e, b);

		Collider c;
		c.half_width = 0.5;
		c.shape = ColliderShape::Circle;
		Entities::AddComponent<Collider>(e, c);

		Entities::AddComponent<Texture>(e, {Imager::Get("Ball"), 1.0f});

		pos_x -= 1.0f;
	}
}

void GravityTest::OnUpdate(double timestep) {}

void GravityTest::OnRender()
{
	/* Draw a Button */
	if (UI::Button(50.0f, 50.0f, 150.0f, 50.0f, "Images/buttons/main_menu.png") == 3)
	{
		logger->info("Switching to the Main Menu...");
		Events::Post<SwitchToMainMenuEvent>({});
	}
}
