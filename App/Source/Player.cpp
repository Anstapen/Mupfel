#include "Player.h"
#include "Types.h"
#include "Imager.h"
#include <string_view>
#include <cmath>

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
	PlayerMovedEvent() {};
	PlayerMovedEvent(float in_velocity_x, float in_velocity_y, std::string_view in_wanted = {})
		: velocity_x(in_velocity_x), velocity_y(in_velocity_y), wanted_animation(in_wanted) {};

	float velocity_x = 0.0f;
	float velocity_y = 0.0f;
	std::string_view wanted_animation{};
};

Player::Player(Mupfel::Registry& registry) : e(registry.CreateEntity()) {}

void Player::Init()
{
	logger = Logger::Create("Player");

	Imager::LoadAnimated("PlayerNaked", "Images/spritesheet.png", {.rows = 1, .columns = 8});

	animations["idle_front"] = {0, 8, 8.0f, 0.0f, true, true};
	animations["idle_back"] = {0, 8, 8.0f, 0.0f, true, true};
	animations["idle_right"] = {0, 8, 8.0f, 0.0f, true, true};
	animations["idle_left"] = {0, 8, 8.0f, 0.0f, true, true};

	auto& registry = Application::GetCurrentRegistry();

	e = Entities::Create();
	Transform p;
	p.pos_z = 0.1f;

	Entities::AddComponent<Transform>(e, p);

	Texture tex;
	tex.scale_x = 2.0f;
	tex.scale_y = 2.0f;
	tex.index = Imager::Get("PlayerNaked");
	Entities::AddComponent<Texture>(e, tex);

	current_anim = "idle_front";
	Entities::AddComponent<Animation>(e, animations.at(current_anim));

	Collider c;
	c.SetBox(1.0f, 0.5f);
	c.SetCapsule(-0.3f, 0.0f, 0.3f, 0.0f, 0.2f);
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

	Input::MapKey<PlayerMovedEvent>(Key::KEY_W, KeyAction::PRESSED, {0.0f, 1.0f, "idle_back"});
	Input::MapKey<PlayerMovedEvent>(Key::KEY_W, KeyAction::RELEASED, {0.0f, -1.0f});

	Input::MapKey<PlayerMovedEvent>(Key::KEY_A, KeyAction::PRESSED, {-1.0f, 0.0f, "idle_left"});
	Input::MapKey<PlayerMovedEvent>(Key::KEY_A, KeyAction::RELEASED, {1.0f, 0.0f});

	Input::MapKey<PlayerMovedEvent>(Key::KEY_S, KeyAction::PRESSED, {0.0f, -1.0f, "idle_front"});
	Input::MapKey<PlayerMovedEvent>(Key::KEY_S, KeyAction::RELEASED, {0.0f, 1.0f});

	Input::MapKey<PlayerMovedEvent>(Key::KEY_D, KeyAction::PRESSED, {1.0f, 0.0f, "idle_right"});
	Input::MapKey<PlayerMovedEvent>(Key::KEY_D, KeyAction::RELEASED, {-1.0f, 0.0f});
}

void Player::UpdateMovement(double timestep)
{
	Mupfel::EventSystem& evt_system = Application::GetCurrentEventSystem();
	auto&				 registry = Application::GetCurrentRegistry();
	static std::string_view wanted = "idle_front";

	CheckPlayerCollisions();

	for (auto& event : evt_system.GetEvents<PlayerMovedEvent>())
	{
		velocity_x += event.velocity_x;
		velocity_y += event.velocity_y;
		if (velocity_x > 0.0f)
		{
			wanted = "idle_right";
		}
		if (velocity_x < 0.0f)
		{
			wanted = "idle_left";
		}
		if (velocity_y > 0.0f)
		{
			wanted = "idle_back";
		}
		if (velocity_y < 0.0f)
		{
			wanted = "idle_front";
		}
	}

	if (!wanted.empty() && wanted != current_anim)
	{
		current_anim = wanted;
		registry.GetComponent<Animation>(e) = animations.at(current_anim);
	}

	Mupfel::Transform& t = registry.GetComponent<Transform>(e);

	float vel_x = velocity_x;
	float vel_y = velocity_y;

	static const float sqrt_2 = sqrtf(2);

	/* Do the cheap calculation first */
	float magnitude_sqrd = powf(vel_x, 2) + powf(vel_y, 2);

	/* For the 0.0f case, we can test for inequality. */
	if (magnitude_sqrd != 0.0f)
	{
		float magnitude = sqrtf(magnitude_sqrd);
		vel_x /= magnitude;
		vel_y /= magnitude;
	}

	Application::SetMovement(e, vel_x * movement_speed, vel_y * movement_speed, 0.0f);
}

void Player::CheckPlayerCollisions(void)
{
	/* Lets check hit events first. */
	for (auto& event : Events::Get<CollisionHitEvent>())
	{
		if (event.a == e || event.b == e)
		{
			/* handle it */
		}
	}
}

void Player::UpdatePlayerMovementSpeed(float speed)
{
	movement_speed = speed;
}
