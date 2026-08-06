#include "Level.h"
#include "Core/Application.h"
#include "ECS/Components/Animation.h"
#include "ECS/Components/Light.h"
#include "ECS/Components/Movement.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Transform.h"
#include <vector>

using namespace Mupfel;

void Level::OnInit() {
	camera.pitch = 1.569051f;
	auto result = Application::LoadBasicImage("Images/dungeon.png")
				 .transform([this](ImageHandle handle) { this->image_map["Map"] = handle; });

	result = Application::LoadAnimatedImage(
				 "Images/Vampires1/With_shadow/Vampires1_Idle_with_shadow.png", {.rows = 4, .columns = 4})
				 .transform([this](ImageHandle handle) { this->image_map["Vampire"] = handle; });

	result = Application::LoadAnimatedImage(
				 "Images/chest.png", {.rows = 3, .columns = 5})
				 .transform([this](ImageHandle handle) { this->image_map["Chest"] = handle; });

	result = Application::LoadAnimatedImage("Images/garg_lava.png", {.rows = 1, .columns = 3})
				 .transform([this](ImageHandle handle) { this->image_map["GargLava"] = handle; });

	result = Application::LoadAnimatedImage("Images/garg_water.png", {.rows = 1, .columns = 3})
				 .transform([this](ImageHandle handle) { this->image_map["GargWater"] = handle; });

	result = Application::LoadAnimatedImage("Images/spikes.png", {.rows = 1, .columns = 4})
				 .transform([this](ImageHandle handle) { this->image_map["Spikes"] = handle; });


	Mupfel::Registry& registry = Application::GetCurrentRegistry();

	// Ground: one large flat quad in the x/y plane, grass tiled ~1 texture per world unit.
	{
		Entity	  e = registry.CreateEntity();
		Transform g;
		g.scale_x = 31.0f;
		g.scale_y = 13.0f;

		registry.AddComponent<Transform>(e, g);

		if (image_map.contains("Map"))
		{
			Texture tex;
			tex.uvScale = 1.0f;
			tex.index = image_map["Map"];
			registry.AddComponent<Texture>(e, tex);
		}
	}

	/* A simple Light */
	{
		Entity	  e = registry.CreateEntity();
		Transform g;
		g.pos_z = 1.0f;

		registry.AddComponent<Transform>(e, g);

		Light l;
		l.ambientStrength = 0.1;
		l.r = 1.0f;
		l.g = 1.0f;
		l.b = 1.0f;

		registry.AddComponent<Light>(e, l);
	}

	/* The 3 different chests */
	{
		Entity	  e = registry.CreateEntity();
		Transform g;
		g.pos_z = 0.08f;

		registry.AddComponent<Transform>(e, g);

		if (image_map.contains("Chest"))
		{
			Texture tex;
			tex.index = image_map["Chest"];
			registry.AddComponent<Texture>(e, tex);
		}

		registry.AddComponent<Animation>(e, {0, 5, 2.0f});
	}

	{
		Entity	  e = registry.CreateEntity();
		Transform g;
		g.pos_x = 1.0f;
		g.pos_z = 0.08f;

		registry.AddComponent<Transform>(e, g);

		if (image_map.contains("Chest"))
		{
			Texture tex;
			tex.index = image_map["Chest"];
			registry.AddComponent<Texture>(e, tex);
		}

		registry.AddComponent<Animation>(e, {5, 5, 2.0f});
	}

	{
		Entity	  e = registry.CreateEntity();
		Transform g;
		g.pos_x = 2.0f;
		g.pos_z = 0.08f;

		registry.AddComponent<Transform>(e, g);

		if (image_map.contains("Chest"))
		{
			Texture tex;
			tex.index = image_map["Chest"];
			registry.AddComponent<Texture>(e, tex);
		}

		registry.AddComponent<Animation>(e, {10, 5, 2.0f});
	}

	/* Two gargs */
	{
		Entity	  e = registry.CreateEntity();
		Transform g;
		g.scale_y = 3.0f;
		g.pos_x = -6.0f;
		g.pos_y = 5.0f;
		g.pos_z = 0.08f;

		registry.AddComponent<Transform>(e, g);

		if (image_map.contains("GargLava"))
		{
			Texture tex;
			tex.index = image_map["GargLava"];
			registry.AddComponent<Texture>(e, tex);
		}

		registry.AddComponent<Animation>(e, {0, 3, 2.0f});
	}

	{
		Entity	  e = registry.CreateEntity();
		Transform g;
		g.scale_y = 3.0f;
		g.pos_x = -5.0f;
		g.pos_y = 5.0f;
		g.pos_z = 0.08f;

		registry.AddComponent<Transform>(e, g);

		if (image_map.contains("GargWater"))
		{
			Texture tex;
			tex.index = image_map["GargWater"];
			registry.AddComponent<Texture>(e, tex);
		}

		registry.AddComponent<Animation>(e, {0, 3, 2.0f});
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
		Entity	  e = registry.CreateEntity();
		Transform g;
		g.pos_x = x;
		g.pos_y = y;
		g.pos_z = 0.08f;

		registry.AddComponent<Transform>(e, g);

		if (image_map.contains("Spikes"))
		{
			Texture tex;
			tex.index = image_map["Spikes"];
			registry.AddComponent<Texture>(e, tex);
		}

		registry.AddComponent<Animation>(e, {0, 4, 1.0f, elapsed});
		elapsed += 0.1f;
	}

	player.Init();

}

void Level::OnUpdate(double timestep) { player.UpdateMovement(timestep); }

void Level::OnRender() {}

void Level::OnSwitchIn() {}

void Level::OnSwitchOut() {}

void Level::Serialize(const std::string& path) {}

void Level::Deserialize(const std::string& path) {}