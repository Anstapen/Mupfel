#include "HelloWorldLayer.h"
#include "Renderer/Text.h"
#include "Core/Application.h"
#include <iostream>

void HelloWorldLayer::OnInit()
{
}

void HelloWorldLayer::OnUpdate(float timestep)
{

	auto &evt_system = Mupfel::Application::GetCurrentEventSystem();

	uint32_t pending_user_inputs = evt_system.GetPendingEvents<Mupfel::UserInputEvent>();

	for (uint32_t i = 0; i < pending_user_inputs; i++)
	{
		auto user_input_event = evt_system.GetEvent<Mupfel::UserInputEvent>(i);

		if (user_input_event)
		{
			std::cout << "Hello World found a User Input Event: " << (uint32_t)user_input_event.value()->input << std::endl;
		}
	}

}

void HelloWorldLayer::OnRender()
{
	Mupfel::Text::RaylibDrawText("Congrats! You created your first window!", 190, 200);
}
