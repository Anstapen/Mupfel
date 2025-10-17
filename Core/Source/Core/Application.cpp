#include "Application.h"
#include "raylib.h"
#include <algorithm>

using namespace Mupfel;

Application& Application::Get()
{
	static Application app;
	return app;
}

Application::Application() :
	window(Window::GetInstance()),
	evt_system(),
	input_manager(evt_system),
	registry(evt_system),
	collision_system(registry, evt_system),
	thread_pool(std::thread::hardware_concurrency())
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

	/* Init Raylib RND number generator */
	SetRandomSeed(997478384U);

	WindowSpecification window_spec;
	window_spec.title = app.spec.name;

	Window::GetInstance().Init(window_spec);

	collision_system.Init();

	/* Add a debug Layer */
	this->PushLayer<DebugLayer>();

	return true;
}

void Application::Stop()
{
	running = false;
}

double Application::GetCurrentTime()
{
	return GetTime();
}

float Mupfel::Application::GetLastFrameTime()
{
	return GetFrameTime();
}

int Mupfel::Application::GetRandomNumber(int min, int max)
{
	return GetRandomValue(min, max);
}

int Mupfel::Application::GetCurrentRenderWidth()
{
	return GetRenderWidth();
}

int Mupfel::Application::GetCurrentRenderHeight()
{
	return GetRenderHeight();
}

bool Mupfel::Application::isDebugModeEnabled()
{
	return Get().debugModeEnabled;
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

ThreadPool& Mupfel::Application::GetCurrentThreadPool()
{
	return Get().thread_pool;
}

void Application::Run()
{
	running = true;

	double lastTime = Application::GetCurrentTime();

	/* Main Loop */
	while (running)
	{
		if (window.ShouldClose())
		{
			Stop();
			break;
		}

		double currentTime = Application::GetCurrentTime();
		double timestep = std::clamp<double>(currentTime - lastTime, 0.001f, 0.1f);
		lastTime = currentTime;

		/* Check for Application related changes */
		for (const auto& evt : evt_system.GetEvents<Mupfel::UserInputEvent>())
		{
			if (evt.input == UserInput::WINDOW_FULLSCREEN)
			{
				window.ToggleFS();
			}

			if (evt.input == UserInput::TOGGLE_DEBUG_MODE)
			{
				std::cout << "Toggled Debug Mode!" << std::endl;
				debugModeEnabled = !debugModeEnabled;
			}

			if (evt.input == UserInput::TOGGLE_MULTI_THREAD_MODE)
			{
				std::cout << "Toggled MT Mode!" << std::endl;
				collision_system.ToggleMultiThreading();
				std::cout << "MT is now: " << (collision_system.IsMultiThreaded() ? "Enabled" : "Disabled") << std::endl;
			}
		}

		/* Update the Collision System */
		collision_system.Update();

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
