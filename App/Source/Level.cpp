#include "Level.h"
#include "Imager.h"
#include "Components/ProximityAnimator.h"
#include "SceneSwitches.h"
#include <vector>

using namespace Mupfel;

void Level::OnInit()
{
	auto load_proximity_animator = [] (Entity e, const nlohmann::json &source)
	{
		ProximityAnimator a;
		a.BuildFromJSON(source);
		Entities::AddComponent<ProximityAnimator>(e, a);
	};
	e_mngr.RegisterComponentLoader("ProximityAnimation", load_proximity_animator);
	e_mngr.Load("Entities/entities.json", nullptr);
	player.Init();
}

void Level::OnUpdate(double timestep)
{
	UpdateUserInputs();
	player.UpdateMovement(timestep);
}

void Level::OnRender()
{
	/* Draw a Button */
	if (UI::Button(50.0f, 50.0f, 150.0f, 50.0f, "Images/buttons/main_menu.png") == 3)
	{
		logger->info("Switching to the Main Menu...");
		Events::Post<SwitchSceneEvent>(SwitchSceneEvent(AppScene::MAIN_MENU));
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
			const ScreenPoint screen_cursor = {static_cast<float>(Input::CursorX()), static_cast<float>(Input::CursorY())};

			const ScreenVector diff = {screen_cursor.x - last_cursor_on_screen.x, screen_cursor.y - last_cursor_on_screen.y};

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
