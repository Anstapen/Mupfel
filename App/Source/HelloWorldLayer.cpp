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

	/*
		Retrieve all the UserInputEvents that were issued the last frame.
	*/
	for (const auto& evt : evt_system.GetEvents<Mupfel::UserInputEvent>())
	{
		std::cout << "Hello World found a User Input Event: " << (uint32_t)evt.input << std::endl;
	}

}

void HelloWorldLayer::OnRender()
{
	Mupfel::Text::RaylibDrawText("Congrats! You created your first window!", 190, 200);
}
