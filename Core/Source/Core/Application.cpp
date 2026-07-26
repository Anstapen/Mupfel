#include "Application.h"
#include "Logger/Logger.h"
#include "Ping/Device.h"
#include "Ping/Ping.h"
#include "Profiler.h"
#include "Renderer/Renderer.h"
#include <algorithm>
#include <chrono>
#include <iostream>

#include "GLFW/glfw3.h"

using Clock = std::chrono::steady_clock;
static const Clock::time_point start_time = Clock::now();

using namespace Mupfel;

Application& Application::Get()
{
	static Application app;
	return app;
}

Application::Application()
	: window(Window::GetInstance()), evt_system(), input_manager(evt_system), registry(evt_system),
	  physics(registry, evt_system), thread_pool(std::thread::hardware_concurrency())
{
}

Application::~Application() {}

bool Application::Init(const ApplicationSpecification& in_spec)
{
	auto& app = Get();
	app.spec = in_spec;

	if (app.spec.name.empty())
	{
		app.spec.name.insert(0, "Application");
	}

	Logger::Init();

	logger = Logger::Create(app.spec.name);
	logger->info("{} initializing...", app.spec.name);

	WindowSpecification window_spec;
	window_spec.title = app.spec.name;

	Window::GetInstance().Init(window_spec);

	if (!Ping::Init())
	{
		logger->error("Failed to initialize the RHI.");
		return false;
	}

	gpu = Ping::Device(Ping::DeviceSpecification(), Window::GetInstance().GetGLFWHandle());

	if (!renderer.Init(gpu.value(), Window::GetInstance()))
	{
		logger->error("Renderer Initialization failed!");
		return false;
	}

	physics.Init();

	debug_layer.OnInit();

	if (!animationSystem.Init())
	{
		logger->error("Animation System Initialization failed!");
		return false;
	}

	frame_count = 0;

	return true;
}

void Application::Stop() { running = false; }

double Application::GetTime() { return std::chrono::duration<double>(Clock::now() - start_time).count(); }

void Mupfel::Application::StartFrameTime() { Get().start_frame_time = GetTime(); }

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

void Mupfel::Application::WaitTime(double time) { std::this_thread::sleep_for(std::chrono::duration<double>(time)); }

float Mupfel::Application::GetLastFrameTime() { return static_cast<float>(Get().last_frame_time); }

int Mupfel::Application::GetCurrentRenderWidth()
{
	int32_t width, height;
	Get().window.GetFramebufferSize(width, height);

	return width;
}

int Mupfel::Application::GetCurrentRenderHeight()
{
	int32_t width, height;
	Get().window.GetFramebufferSize(width, height);

	return height;
}

bool Mupfel::Application::isDebugModeEnabled() { return Get().debugModeEnabled; }

EventSystem& Application::GetCurrentEventSystem() { return Get().evt_system; }

InputManager& Mupfel::Application::GetCurrentInputManager() { return Get().input_manager; }

Registry& Mupfel::Application::GetCurrentRegistry() { return Get().registry; }

PhysicsSimulation& Mupfel::Application::GetCurrentPhysicsSim() { return Get().physics; }

Expected<ImageHandle> Mupfel::Application::LoadBasicImage(const std::string path)
{
	return Get().image_manager.Load(Get().gpu.value(), path);
}

Expected<ImageHandle> Mupfel::Application::LoadAnimatedImage(const std::string path, const ImageSpecification& spec)
{
	return Get().image_manager.LoadAnimated(Get().gpu.value(), path, spec);
}

Expected<std::vector<ImageHandle>>
Mupfel::Application::LoadSpriteSheetImages(const std::string path, const ImageSpecification& spec)
{
	return Get().image_manager.LoadSpriteSheet(Get().gpu.value(), path, spec);
}

ThreadPool& Mupfel::Application::GetCurrentThreadPool() { return Get().thread_pool; }

void Mupfel::Application::SetTimeScale(double time_scale) { Get().physics.SetTimeMultiplier(time_scale); }

void Mupfel::Application::TogglePhysicsSingleStep() { Get().physics.ToggleSingleStep(); }

void Mupfel::Application::PhysicsStep() { Get().physics.Step(); }

uint64_t Mupfel::Application::GetFrameCount() { return Get().frame_count; }

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
		window.PollEvents();
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
			ProfilingSample prof("Animation Update");
			/* Update the Collision System */
			animationSystem.Update(timestep);
		}

		{
			ProfilingSample prof("Engine Renderer Begin");
			renderer.Begin(gpu.value(), Window::GetInstance(), timestep);
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
			ProfilingSample prof("Engine Renderer End");
			renderer.End(gpu.value(), Window::GetInstance(), timestep);
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

ImageManager& Mupfel::Application::GetCurrentImageManager() { return Get().image_manager; }
