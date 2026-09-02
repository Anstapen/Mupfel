#include "Chest.h"
#include "Imager.h"
#include "Types.h"

using namespace Mupfel;

std::unordered_map<std::string, Mupfel::Animation> Chest::animations = {
	{"opening", {0, 3, 8.0f, 0.0f, false, true}},
	{"closing", {2, 3, 8.0f, 0.0f, false, true}},
	{"opened", {2, 1, 8.0f, 0.0f, false, false}},
	{"closed", {0, 1, 8.0f, 0.0f, false, false}}};

Chest::Chest(const std::string& texture_handle, float pos_x, float pos_y) : e(Entities::Create())
{
	/* Add the baseline components that every chest has. */
	Transform g{pos_x, pos_y, 0.08f, 0.0f};

	Entities::AddComponent<Transform>(e, g);

	Entities::AddComponent<Texture>(e, {Imager::Get(texture_handle), 1.0f});

	Entities::AddComponent<Animation>(e, animations["closed"]);

	Entities::AddComponent<Body>(e, {});

	Sensor s;
	s.SetCircle(1.0f);
	s.report_events = true;
	s.category = ColliderType::Furniture;
	s.mask = ColliderType::Player;
	Entities::AddComponent<Sensor>(e, s);

	Collider c;
	c.SetBox(1.0f, 0.8f);
	c.offset_y = -0.1f;
	Entities::AddComponent<Collider>(e, c);
}

Chest::~Chest() { Entities::Destroy(e); }

void Chest::CheckEvents()
{
	bool first_visitor_entered = false;
	bool last_visitor_exited = false;
	if (Application::HasPhysicsEvents(e))
	{
		/* Search for the event */
		for (auto& event : Events::Get<SensorEnteredEvent>())
		{
			if (event.sensor == e)
			{
				if (current_visitors == 0)
				{
					first_visitor_entered = true;
					animation_queue.push("opening");
				}
				current_visitors++;
			}
		}

		for (auto& event : Events::Get<SensorExitedEvent>())
		{
			if (event.sensor == e)
			{
				if (current_visitors == 1)
				{
					last_visitor_exited = true;
					animation_queue.push("closing");
				}
				current_visitors--;
			}
		}
	}

	/* Check the animation queue */
	if (!animation_queue.empty() && Entities::GetComponent<Animation>(e).IsFinished())
	{

		Entities::GetComponent<Animation>(e) = animations[animation_queue.back()];
		animation_queue.pop();
	}
}
