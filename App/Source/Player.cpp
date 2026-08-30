#include "Player.h"
#include <string_view>
#include "Types.h"

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
	logger = Logger::Create("Player");
	auto result = Application::LoadAnimatedImage(
					  "Images/Vampires1/With_shadow/Vampires1_Idle_with_shadow.png", {.rows = 4, .columns = 4})
					  .transform([this](ImageHandle handle) { this->image_map["Vampire"] = handle; });

	animations["idle_front"] = {0, 4, 1.0f, 0.0f, true, true};
	animations["idle_back"] = {4, 4, 1.0f, 0.0f, true, true};
	animations["idle_left"] = {8, 4, 1.0f, 0.0f, true, true};
	animations["idle_right"] = {12, 4, 1.0f, 0.0f, true, true};

	auto& registry = Application::GetCurrentRegistry();

	e = Entities::Create();
	Transform p;
	p.pos_z = 0.1f;
	
	Entities::AddComponent<Transform>(e, p);
	if (image_map.contains("Vampire"))
	{
		Texture tex;
		tex.scale_x = 5.0f;
		tex.scale_y = 5.0f;
		tex.index = image_map["Vampire"];
		Entities::AddComponent<Texture>(e, tex);
	}

	current_anim = "idle_front";
	Entities::AddComponent<Animation>(e, animations.at(current_anim));

	Collider c;
	c.shape = ColliderShape::Box;
	c.half_height = 0.25;
	c.half_width = 0.5;
	c.offset_y = -0.75;
	c.report_contacts = true;
	c.report_hit_events = true;
	c.report_sensor_events = true;
	c.category = ColliderType::Player;

	Entities::AddComponent<Collider>(e, c);


	Body b;
	b.type = BodyType::Dynamic;
	b.fixed_rotation = true;

	Entities::AddComponent<Body>(e, b);

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

	CheckPlayerCollisions();
	/*
		The InputManager emits the same event for PRESSED and RELEASED (Binding::emitter drops the
		KeyAction), so a key's press/release cycle is tracked by toggling the flag on every event.
	*/
	if (evt_system.GetPendingEvents<PlayerMovedEvent>())
	{
		movement_changed = true;
	}
	for (auto& event : evt_system.GetEvents<PlayerMovedEvent>())
	{
		switch (event.movement)
		{
		case PlayerMovement::FORWARD:
			moving_up = !moving_up;
			break;
		case PlayerMovement::BACKWARDS:
			moving_down = !moving_down;
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
		wanted = "idle_back";
	}
	else if (moving_down)
	{
		wanted = "idle_front";
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

	if (movement_changed)
	{
		/* Recalculate the player movement */
		float vel_x = 0.0f, vel_y = 0.0f;

		constexpr float vel = 3.0f;

		if (moving_right)
		{
			vel_x = vel;
		}

		if (moving_left)
		{
			vel_x = -vel;
		}

		if (moving_up)
		{
			vel_y = vel;
		}

		if (moving_down)
		{
			vel_y = -vel;
		}

		Application::SetMovement(e, vel_x, vel_y, 0.0f);

		movement_changed = false;
	}
}

void Player::CheckPlayerCollisions(void) { 
	/* Lets check hit events first. */
	for (auto& event : Events::Get<CollisionHitEvent>())
	{
		if (event.a == e || event.b == e)
		{
			logger->info("Player collided with something!");
		}
	}

	/* After that sensor begin */
	for (auto& event : Events::Get<SensorEnteredEvent>())
	{
		if (event.visitor == e)
		{
			logger->info("Player is visiting a sensor!");

			/* If the sensor has an animation, reset it (if that animation has finished). */
			if (Entities::HasComponent<Animation>(event.sensor) && Entities::GetComponent<Animation>(event.sensor).IsFinished())
			{
				Entities::GetComponent<Animation>(event.sensor).Reset();
			}
		}
	}
}
