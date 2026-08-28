#include "Level.h"
#include "Imager.h"
#include "SceneSwitches.h"
#include <vector>

using namespace Mupfel;

void Level::OnInit()
{
	camera.pitch = 1.569051f;

	Imager::Load("Map", "Images/dungeon.png");
	Imager::LoadAnimated(
		"Vampire", "Images/Vampires1/With_shadow/Vampires1_Idle_with_shadow.png", {.rows = 4, .columns = 4});

	Imager::LoadAnimated("Chest", "Images/chest.png", {.rows = 3, .columns = 5});
	Imager::LoadAnimated("GargLava", "Images/garg_lava.png", {.rows = 1, .columns = 3});
	Imager::LoadAnimated("GargWater", "Images/garg_water.png", {.rows = 1, .columns = 3});
	Imager::LoadAnimated("Spikes", "Images/spikes.png", {.rows = 1, .columns = 4});

	// Ground: one large flat quad in the x/y plane, grass tiled ~1 texture per world unit.
	{
		Entity	  e = Entities::Create();
		Entities::AddComponent<Transform>(e, {});
		Entities::AddComponent<Texture>(e, {Imager::Get("Map"), 31.0f, 13.0f});
	}

	/* A simple Light */
	{
		Entity	  e = Entities::Create();
		Transform g;
		g.pos_z = 1.0f;

		Entities::AddComponent<Transform>(e, g);

		Light l;
		l.ambientStrength = 0.1;
		l.r = 1.0f;
		l.g = 1.0f;
		l.b = 1.0f;

		Entities::AddComponent<Light>(e, l);
	}

	/* The 3 different chests */
	{
		Entity	  e = Entities::Create();
		Transform g;
		g.pos_z = 0.08f;

		Entities::AddComponent<Transform>(e, g);

		Entities::AddComponent<Texture>(e, {Imager::Get("Chest"), 1.0f});

		Entities::AddComponent<Animation>(e, {0, 5, 2.0f});
	}

	{
		Entity	  e = Entities::Create();
		Transform g;
		g.pos_x = 1.0f;
		g.pos_z = 0.08f;

		Entities::AddComponent<Transform>(e, g);
		Entities::AddComponent<Texture>(e, {Imager::Get("Chest"), 1.0f});

		Entities::AddComponent<Animation>(e, {5, 5, 2.0f});
	}

	{
		Entity	  e = Entities::Create();
		Transform g;
		g.pos_x = 2.0f;
		g.pos_z = 0.08f;

		Entities::AddComponent<Transform>(e, g);

		Entities::AddComponent<Texture>(e, {Imager::Get("Chest"), 1.0f});

		Entities::AddComponent<Animation>(e, {10, 5, 2.0f});
	}

	/* Two gargs */
	{
		Entity	  e = Entities::Create();
		Transform g;
		g.pos_x = -6.0f;
		g.pos_y = 5.0f;
		g.pos_z = 0.08f;

		Entities::AddComponent<Transform>(e, g);
		Entities::AddComponent<Texture>(e, {Imager::Get("GargLava"), 1.0f, 3.0f});

		Entities::AddComponent<Animation>(e, {0, 3, 2.0f});
	}

	{
		Entity	  e = Entities::Create();
		Transform g;
		g.pos_x = -5.0f;
		g.pos_y = 5.0f;
		g.pos_z = 0.08f;

		Entities::AddComponent<Transform>(e, g);
		Entities::AddComponent<Texture>(e, {Imager::Get("GargWater"), 1.0f, 3.0f});

		Entities::AddComponent<Animation>(e, {0, 3, 2.0f});
	}

	/* A bunch of spikes */

	std::vector<std::pair<float, float>> spike_positions;

	spike_positions.push_back({-4.0f, 0.0f});
	spike_positions.push_back({-4.0f, -1.0f});
	spike_positions.push_back({-4.0f, 1.0f});
	spike_positions.push_back({-4.0f, 2.0f});
	spike_positions.push_back({-5.0f, -1.0f});
	spike_positions.push_back({-5.0f, 2.0f});
	spike_positions.push_back({-6.0f, -1.0f});
	spike_positions.push_back({-6.0f, 2.0f});
	spike_positions.push_back({-7.0f, 0.0f});
	spike_positions.push_back({-7.0f, -1.0f});
	spike_positions.push_back({-7.0f, 1.0f});
	spike_positions.push_back({-7.0f, 2.0f});

	float elapsed = 0.0f;

	for (auto& [x, y] : spike_positions)
	{
		Entity	  e = Entities::Create();
		Transform g;
		g.pos_x = x;
		g.pos_y = y;
		g.pos_z = 0.08f;

		Entities::AddComponent<Transform>(e, g);

		Collider c;
		c.is_sensor = true;
		Entities::AddComponent<Collider>(e, c);
		Body b;
		b.fixed_rotation = true;
		b.type = BodyType::Static;
		Entities::AddComponent<Body>(e, b);
		Entities::AddComponent<Texture>(e, {Imager::Get("Spikes"), 1.0f});

		Entities::AddComponent<Animation>(e, {0, 4, 1.0f, elapsed});
		elapsed += 0.1f;
	}

	player.Init();
}

void Level::OnUpdate(double timestep) { player.UpdateMovement(timestep); }

void Level::OnRender()
{
	/* Draw a Button */
	if (UI::Button(50.0f, 50.0f, 150.0f, 50.0f, "Images/buttons/main_menu.png") == 3)
	{
		logger->info("Switching to the Main Menu...");
		Events::Post<SwitchToMainMenuEvent>({});
	}
}