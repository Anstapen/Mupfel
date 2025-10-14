#include "HelloWorldLayer.h"
#include "Renderer/Text.h"
#include "Core/Application.h"
#include <iostream>
#include "ECS/View.h"

using namespace Mupfel;

struct Velocity {
	float x, y;
};

void HelloWorldLayer::OnInit()
{
	
}

void HelloWorldLayer::OnUpdate(float timestep)
{

	auto &evt_system = Mupfel::Application::GetCurrentEventSystem();

	/*
		Retrieve all the UserInputEvents that were issued the last frame.
	*/
	for (const auto& evt : evt_system.GetEvents<Mupfel::UserInputEvent>())
	{
		std::cout << "Hello World found a User Input Event: " << (uint32_t)evt.input << std::endl;

		/* If Left-Mouseclick is pressed, iterate over the entities */
		if (evt.input == Mupfel::UserInput::LEFT_MOUSE_CLICK)
		{
			Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();

			auto view = reg.view<Velocity>();

			for (auto [entity, velocity] : view)
			{
				std::cout << "Entity " << entity.Index()
					<< " -> (" << velocity.x << ", " << velocity.y << ")\n";
			}
		}

		/* If Right-Mouseclick is pressed, create new entites */
		if (evt.input == Mupfel::UserInput::RIGHT_MOUSE_CLICK)
		{
			Mupfel::Registry& reg = Mupfel::Application::GetCurrentRegistry();

			Mupfel::Entity ent = reg.CreateEntity();
			
			std::cout << "Created Entity with index:" << ent.Index() << std::endl;

			/* Add velocity component to the entity */
			reg.AddComponent<Velocity>(ent, { 1.0f, 2.0f });

			std::cout << "Added Component with ID " << CompUtil::GetComponentTypeID<Velocity>() << std::endl;
		}
	}

}

void HelloWorldLayer::OnRender()
{
	Mupfel::Text::RaylibDrawText("Congrats! You created your first window!", 190, 200);
}
