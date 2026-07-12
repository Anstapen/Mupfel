#include "Ping/Device.h"
#include "Application.h"
#include <algorithm>
#include "Profiler.h"
#include "Renderer/Renderer.h"
#include <iostream>
#include <chrono>
#include "Ping/Ping.h"
#include "Logger/Logger.h"


#include "GLFW/glfw3.h"

using Clock = std::chrono::steady_clock;
static const Clock::time_point start_time = Clock::now();


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
	physics(registry, evt_system),
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

	Logger::Init();

	WindowSpecification window_spec;
	window_spec.title = app.spec.name;

	Window::GetInstance().Init(window_spec);

	if (!Ping::Init())
	{
		return false;
	}
	gpu = Ping::Device(Ping::DeviceSpecification(), Window::GetInstance().GetGLFWHandle());

	renderer.Init(gpu.value(), Window::GetInstance());

	physics.Init();

	debug_layer.OnInit();

	frame_count = 0;

	return true;
}

void Application::Stop()
{
	running = false;
}

double Application::GetTime()
{
	return std::chrono::duration<double>(Clock::now() - start_time).count();
}

void Mupfel::Application::StartFrameTime()
{
	Get().start_frame_time = GetTime();
}

void Mupfel::Application::EndFrameTime()
{
	double current_time = GetTime();
	double frame_time = current_time - Get().start_frame_time;

	double wait_time = (1.0f / 144.0f) - frame_time;

	if (wait_time > 0.0f)
	{
		WaitTime((float)wait_time);
		current_time = GetTime();
		frame_time = (float)(current_time - Get().start_frame_time);
	}

	Get().last_frame_time = frame_time;
}

void Mupfel::Application::WaitTime(double time)
{
	std::this_thread::sleep_for(std::chrono::duration<double>(time));
}

float Mupfel::Application::GetLastFrameTime()
{
	return static_cast<float>(Get().last_frame_time);
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

PhysicsSimulation& Mupfel::Application::GetCurrentPhysicsSim()
{
	return Get().physics;
}

ThreadPool& Mupfel::Application::GetCurrentThreadPool()
{
	return Get().thread_pool;
}

void Mupfel::Application::SetTimeScale(double time_scale)
{
	Get().physics.SetTimeMultiplier(time_scale);
}

void Mupfel::Application::TogglePhysicsSingleStep()
{
	Get().physics.ToggleSingleStep();
}

void Mupfel::Application::PhysicsStep()
{
	Get().physics.Step();
}

uint64_t Mupfel::Application::GetFrameCount()
{
	return Get().frame_count;
}


static GLuint gpuTimerQuery = 0;


void Application::Run()
{
	running = true;

	double lastTime = Application::GetTime();

	/* Main Loop */
	while (running)
	{
		if (window.ShouldClose())
		{
			Stop();
			break;
		}
		Application::StartFrameTime();
		frame_count++;
		ProfilingSample prof("Application::Run()");

		double currentTime = Application::GetTime();
		double timestep = std::clamp<double>(currentTime - lastTime, 0.001f, 0.1f);
		lastTime = currentTime;

		{
			ProfilingSample prof("Application::Run(): Check ");
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
			}
		}
		
		{
			ProfilingSample prof("Layers - OnUpdate ");
			/* Update all layers */
			for (const std::unique_ptr<Layer>& layer : layerStack)
			{
				layer->OnUpdate(timestep);
			}
			debug_layer.OnUpdate(timestep);
		}
		
		{
			ProfilingSample prof("Physics Update");
			/* Update the Collision System */
			physics.Update(timestep);
		}

		

		{
			ProfilingSample prof("Engine Renderer");
			renderer.RenderNextFrame(gpu.value(), Window::GetInstance());
		}
		
		{
			ProfilingSample prof("Layer Rendering");
			for (const std::unique_ptr<Layer>& layer : layerStack)
			{
				layer->OnRender();
			}
		}

		{
			ProfilingSample prof1("DebugLayer");

			if (debugModeEnabled)
			{
				/* Make sure the debug Layer is Rendered last */
				debug_layer.OnRender();
				Profiler::Clear();
			}

		}

		{
			ProfilingSample prof2("EndFrame");			
		}

		{
			ProfilingSample prof2("Event System Update");
			/* Update the EventSystem */
			evt_system.Update();
		}
		
		{
			ProfilingSample prof2("Input Manager Update");
			input_manager.Update(timestep);
		}
		
		Application::EndFrameTime();
	}

	/* Exited Main Loop, clean everything up */
	DeInit();
}


void Application::DeInit()
{
	physics.DeInit();
	gpu.value().WaitForCommands();
	Ping::Shutdown();
}