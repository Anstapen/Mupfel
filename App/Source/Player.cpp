#include "Player.h"
#include "Core/Application.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Transform.h"
#include <string_view>

using namespace Mupfel;

enum class PlayerMovement
{
	NONE,
	FORWARD,
	BACKWARDS,
	LEFT,
	RIGHT
};

class PlayerMovedEvent : public Mupfel::Event
{
public:
	PlayerMovedEvent() : movement(PlayerMovement::NONE) {};
	PlayerMovedEvent(PlayerMovement in_movement) : movement(in_movement) {};

	PlayerMovement movement = PlayerMovement::NONE;
};

Player::Player(Mupfel::Registry& registry) : e(registry.CreateEntity()) {}

void Player::Init()
{
	auto result = Application::LoadAnimatedImage(
					  "Images/Vampires1/With_shadow/Vampires1_Idle_with_shadow.png", {.rows = 4, .columns = 4})
					  .transform([this](ImageHandle handle) { this->image_map["Vampire"] = handle; });

	animations["idle_front"] = {0, 4, 4.0f};
	animations["idle_back"] = {4, 4, 4.0f};
	animations["idle_left"] = {8, 4, 4.0f};
	animations["idle_right"] = {12, 4, 4.0f};

	auto& registry = Application::GetCurrentRegistry();

	e = registry.CreateEntity();
	Transform p;
	p.pos_y = 2.0f;
	p.scale_x = 5.0f;
	p.scale_y = 5.0f;
	registry.AddComponent<Transform>(e, p);
	if (image_map.contains("Vampire"))
	{
		Texture tex;
		tex.index = image_map["Vampire"];
		registry.AddComponent<Texture>(e, tex);
	}

	current_anim = "idle_front";
	registry.AddComponent<Animation>(e, animations.at(current_anim));

	Mupfel::InputManager& input_manager = Mupfel::Application::GetCurrentInputManager();
	input_manager.MapKeyboardButton<PlayerMovedEvent>(
		Key::KEY_W, KeyAction::PRESSED | KeyAction::RELEASED, {PlayerMovement::FORWARD});
	input_manager.MapKeyboardButton<PlayerMovedEvent>(
		Key::KEY_A, KeyAction::PRESSED | KeyAction::RELEASED, {PlayerMovement::LEFT});
	input_manager.MapKeyboardButton<PlayerMovedEvent>(
		Key::KEY_S, KeyAction::PRESSED | KeyAction::RELEASED, {PlayerMovement::BACKWARDS});
	input_manager.MapKeyboardButton<PlayerMovedEvent>(
		Key::KEY_D, KeyAction::PRESSED | KeyAction::RELEASED, {PlayerMovement::RIGHT});
}

void Player::UpdateMovement(double timestep)
{
	Mupfel::EventSystem& evt_system = Application::GetCurrentEventSystem();
	auto&				 registry = Application::GetCurrentRegistry();
	/*
		The InputManager emits the same event for PRESSED and RELEASED (Binding::emitter drops the
		KeyAction), so a key's press/release cycle is tracked by toggling the flag on every event.
	*/
	for (auto& event : evt_system.GetEvents<PlayerMovedEvent>())
	{
		switch (event.movement)
		{
		case PlayerMovement::FORWARD:
			moving_down = !moving_down;
			break;
		case PlayerMovement::BACKWARDS:
			moving_up = !moving_up;
			break;
		case PlayerMovement::LEFT:
			moving_left = !moving_left;
			break;
		case PlayerMovement::RIGHT:
			moving_right = !moving_right;
			break;
		default:
			break;
		}
	}

	/*
		Pick the sequence from the resulting direction, not from the key edge, so releasing one of
		two held keys still leaves the player facing where it actually moves. The camera sits on -y
		looking towards +y, so walking up the screen (+y) shows the back and walking down (-y) shows
		the front. Standing still keeps the last facing.

		Assigning an Animation resets its elapsed time, so only touch the component on a change --
		re-assigning every frame would pin the sprite to frame 0.
	*/
	std::string_view wanted;
	if (moving_up)
	{
		wanted = "idle_front";
	}
	else if (moving_down)
	{
		wanted = "idle_back";
	}
	else if (moving_left)
	{
		wanted = "idle_left";
	}
	else if (moving_right)
	{
		wanted = "idle_right";
	}

	if (!wanted.empty() && wanted != current_anim)
	{
		current_anim = wanted;
		registry.GetComponent<Animation>(e) = animations.at(current_anim);
	}

	Mupfel::Transform& t = registry.GetComponent<Transform>(e);

	if (moving_right)
	{
		t.pos_x += 10.0f * timestep;
	}
	if (moving_left)
	{
		t.pos_x -= 10.0f * timestep;
	}
	if (moving_up)
	{
		t.pos_y -= 10.0f * timestep;
	}
	if (moving_down)
	{
		t.pos_y += 10.0f * timestep;
	}
}
