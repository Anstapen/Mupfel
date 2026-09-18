#include "Level.h"
#include "GameObjects/Chest.h"
#include "GameObjects/Spike.h"
#include "Imager.h"
#include "SceneSwitches.h"
#include "Types.h"
#include <vector>

using namespace Mupfel;

void Level::OnInit()
{

	if (!mngr)
	{
		logger->warn("No resource pack was loaded...");
	}
	Imager::Load("Map", "Images/dungeon.png", mngr.get());

	Imager::LoadAnimated("NormalChest", "Images/normal_chest.png", {.rows = 1, .columns = 5}, mngr.get());
	Imager::LoadAnimated("FilledChest", "Images/filled_chest.png", {.rows = 1, .columns = 5}, mngr.get());
	Imager::LoadAnimated("MonsterChest", "Images/monster_chest.png", {.rows = 1, .columns = 5}, mngr.get());
	Imager::LoadAnimated("GargLava", "Images/garg_lava.png", {.rows = 1, .columns = 3}, mngr.get());
	Imager::LoadAnimated("GargWater", "Images/garg_water.png", {.rows = 1, .columns = 3}, mngr.get());
	Imager::LoadAnimated("Spikes", "Images/spikes.png", {.rows = 1, .columns = 7}, mngr.get());

	{
		Entity e = Entities::Create();
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
		l.ambientStrength = 0.1f;
		l.r = 1.0f;
		l.g = 1.0f;
		l.b = 1.0f;

		Entities::AddComponent<Light>(e, l);
	}

	interactables.push_back(std::make_unique<Chest>("NormalChest", -5.0f, 0.0f));
	interactables.push_back(std::make_unique<Chest>("NormalChest", -6.0f, 0.0f));
	interactables.push_back(std::make_unique<Chest>("NormalChest", -5.0f, 1.0f));
	interactables.push_back(std::make_unique<Chest>("NormalChest", -6.0f, 1.0f));

	/* Two gargs */
	{
		Entity	  e = Entities::Create();
		Transform g;
		g.pos_x = -6.0f;
		g.pos_y = 5.0f;
		g.pos_z = 0.08f;

		Entities::AddComponent<Transform>(e, g);
		Entities::AddComponent<Texture>(e, {Imager::Get("GargLava"), 1.0f, 3.0f});

		Entities::AddComponent<Animation>(e, {0, 3, 2.0f, 0.0f, true, true});
	}

	{
		Entity	  e = Entities::Create();
		Transform g;
		g.pos_x = -5.0f;
		g.pos_y = 5.0f;
		g.pos_z = 0.08f;

		Entities::AddComponent<Transform>(e, g);
		Entities::AddComponent<Texture>(e, {Imager::Get("GargWater"), 1.0f, 3.0f});

		Entities::AddComponent<Animation>(e, {0, 3, 2.0f, 0.0f, true, true});
	}

	/* A bunch of spikes */

	std::vector<std::pair<float, float>> spike_positions;

	spike_positions.emplace_back(-4.0f, 0.0f);
	spike_positions.emplace_back(-4.0f, -1.0f);
	spike_positions.emplace_back(-4.0f, 1.0f);
	spike_positions.emplace_back(-4.0f, 2.0f);
	spike_positions.emplace_back(-5.0f, -1.0f);
	spike_positions.emplace_back(-5.0f, 2.0f);
	spike_positions.emplace_back(-6.0f, -1.0f);
	spike_positions.emplace_back(-6.0f, 2.0f);
	spike_positions.emplace_back(-7.0f, 0.0f);
	spike_positions.emplace_back(-7.0f, -1.0f);
	spike_positions.emplace_back(-7.0f, 1.0f);
	spike_positions.emplace_back(-7.0f, 2.0f);

	for (auto& [x, y] : spike_positions)
	{
		interactables.push_back(std::make_unique<Spike>("Spikes", x, y));
	}

	player.Init();
}

void Level::OnUpdate(double timestep)
{
	UpdateUserInputs();
	for (auto& i : interactables)
	{
		i->CheckEvents();
	}
	player.UpdateMovement(timestep);
}

void Level::OnRender()
{
	/* Draw a Button */
	if (UI::Button(50.0f, 50.0f, 150.0f, 50.0f, "Images/buttons/main_menu.png") == 3)
	{
		logger->info("Switching to the Main Menu...");
		Events::Post<SwitchToMainMenuEvent>({});
	}
}

void Level::UpdateUserInputs()
{
	static bool		   movable_camera = false;
	static ScreenPoint last_cursor_on_screen = {
		static_cast<float>(Input::CursorX()), static_cast<float>(Input::CursorY())};
	static float last_camera_x = camera.target_x;
	static float last_camera_y = camera.target_y;
	/* First, check mouse movement */
	for (auto& event : Events::Get<UserInputEvent>())
	{
		if (event.input == UserInput::RIGHT_MOUSE_CLICK && event.action == KeyAction::PRESSED)
		{
			/* Mouse button has been pressed this frame */
			movable_camera = true;
			last_cursor_on_screen.x = Input::CursorX();
			last_cursor_on_screen.y = Input::CursorY();
		}

		if (event.input == UserInput::RIGHT_MOUSE_CLICK && event.action == KeyAction::RELEASED)
		{
			movable_camera = false;
			/* Mouse button has been pressed this frame */
			ScreenPoint screen_cursor = {static_cast<float>(Input::CursorX()), static_cast<float>(Input::CursorY())};

			ScreenVector diff = {screen_cursor.x - last_cursor_on_screen.x, screen_cursor.y - last_cursor_on_screen.y};

			auto world_diff = camera.ScreenToWorldVector(diff.x, diff.y);

			if (world_diff)
			{
				last_camera_x = last_camera_x - world_diff.value().x;
				last_camera_y = last_camera_y - world_diff.value().y;
			}
		}

		if (event.input == UserInput::SCROLLWHEEL_UP)
		{
			if (camera.distance > 1.0f)
				camera.distance -= 1.0f;
		}

		if (event.input == UserInput::SCROLLWHEEL_DOWN)
		{
			camera.distance += 1.0f;
		}
	}

	/* Update the camera if needed. */
	if (movable_camera)
	{

		ScreenPoint screen_cursor = {static_cast<float>(Input::CursorX()), static_cast<float>(Input::CursorY())};

		ScreenVector diff = {screen_cursor.x - last_cursor_on_screen.x, screen_cursor.y - last_cursor_on_screen.y};

		auto world_diff = camera.ScreenToWorldVector(diff.x, diff.y);

		if (world_diff)
		{
			camera.target_x = last_camera_x - world_diff.value().x;
			camera.target_y = last_camera_y - world_diff.value().y;
		}
	}
}
