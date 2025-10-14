#include "Application.h"
#include "raylib.h"
#include <algorithm>

using namespace Mupfel;

Application& Application::Get()
{
	static Application app;
	return app;
}

Application::Application() : window(Window::GetInstance()), evt_system(), input_manager(evt_system)
{
}

Application::~Application()
{
}

bool Application::Init(const ApplicationSpecification& in_spec)
{
	auto &app = Get();
	app.spec = in_spec;

	if (app.spec.name.empty())
	{
		app.spec.name.insert(0, "Application");
	}

	WindowSpecification window_spec;
	window_spec.title = app.spec.name;

	Window::GetInstance().Init(window_spec);

	return true;
}

void Application::Stop()
{
	running = false;
}

float Application::GetCurrentTime()
{
	return GetTime();
}

EventSystem& Application::GetCurrentEventSystem()
{
	return Get().evt_system;
}

InputManager& Mupfel::Application::GetCurrentInputManager()
{
	return Get().input_manager;
}

Registry& Mupfel::Application::GetCurrentRegistry()
{
	return Get().registry;
}

void Application::Run()
{
	running = true;

	float lastTime = Application::GetCurrentTime();

	/* Main Loop */
	while (running)
	{
		if (window.ShouldClose())
		{
			Stop();
			break;
		}

		float currentTime = Application::GetCurrentTime();
		float timestep = std::clamp<float>(currentTime - lastTime, 0.001f, 0.1f);
		lastTime = currentTime;

		/* Update all layers */
		for (const std::unique_ptr<Layer>& layer : layerStack)
		{
			layer->OnUpdate(timestep);
		}

		window.StartFrame();

		for (const std::unique_ptr<Layer>& layer : layerStack)
		{
			layer->OnRender();
		}

		window.EndFrame();

		/* Update the EventSystem */
		evt_system.Update();
		input_manager.Update(timestep);
	}
}
