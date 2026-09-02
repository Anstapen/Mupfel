#include "Spike.h"
#include "Imager.h"
#include "Types.h"

using namespace Mupfel;

std::unordered_map<std::string, Mupfel::Animation> Spike::animations = {
	{"no_spikes", {0, 1, 1.0f, 0.0f, false, false}},
	{"spikes", {0, 3, 1.0f, 0.0f, false, false}},
	{"extenting", {0, 4, 10.0f, 0.0f, false, true}},
	{"retracting", {3, 4, 10.0f, 0.0f, false, true}}};

Spike::Spike(const std::string& texture_handle, float pos_x, float pos_y) : e(Entities::Create())
{
	/* Add the baseline components that every spike has. */
	Transform g{pos_x, pos_y, 0.08f, 0.0f};

	Entities::AddComponent<Transform>(e, g);

	Entities::AddComponent<Texture>(e, {Imager::Get(texture_handle), 1.0f});

	Entities::AddComponent<Animation>(e, animations["no_spikes"]);

	Entities::AddComponent<Body>(e, {});

	Sensor s;
	s.SetBox(1.0f, 1.0f);
	s.report_events = true;
	s.category = ColliderType::GroundObject;
	s.mask = ColliderType::Player;
	Entities::AddComponent<Sensor>(e, s);
}

Spike::~Spike() { Entities::Destroy(e); }

void Spike::CheckEvents()
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
					animation_queue.push("extenting");
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
					animation_queue.push("retracting");
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