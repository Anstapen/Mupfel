#include "Application.h"
#include "Logger/Logger.h"
#include "Ping/Device.h"
#include "Ping/Ping.h"
#include "Profiler.h"
#include "Renderer/Renderer.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include "ECS/Registry.h"

#include "DefaultScene.h"

#include "GLFW/glfw3.h"

using Clock = std::chrono::steady_clock;
static const Clock::time_point start_time = Clock::now();

using namespace Mupfel;

Application& Application::Get()
{
	static Application app;
	return app;
}

void Mupfel::Application::QueueSceneSwitch(SceneHandle handle)
{
	auto& app = Get();
	if (handle >= Scene::MAX_SCENES)
	{
		return;
	}
	
	if (!app.scenes[handle])
	{
		return;
	}

	app.queued_scene = handle;
}

SceneHandle Mupfel::Application::GetCurrentSceneHandle() { return Get().current_scene; }

Application::Application()
	: window(Window::GetInstance()), evt_system(), input_manager(evt_system),
	  thread_pool(std::thread::hardware_concurrency()), registry(evt_system, thread_pool),
	  physics(registry, evt_system)
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

	/* Try to get the config. */
	configManager.LoadConfig("mupfel.ini");

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

	/* Add Scene 0 */
	SceneHandle first_handle = CreateScene<DefaultScene>("DefaultScene");

	/* We should be the first ones to create a Scene! */
	assert(first_handle == 0);

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

#if 0
	double wait_time = (1.0f / 500.0f) - frame_time;

	if (wait_time > 0.0f)
	{
		WaitTime((float)wait_time);
		current_time = GetTime();
		frame_time = (float)(current_time - Get().start_frame_time);
	}
#endif
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

bool Mupfel::Application::IsWindowMinimized() { return Get().window.IsMinimized(); }

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

void Mupfel::Application::SwitchScene(SceneHandle new_scene)
{
	auto& app = Get();
	if (new_scene >= Scene::MAX_SCENES)
	{
		return;
	}

	if (!app.scenes[new_scene])
	{
		return;
	}

	app.scenes[app.current_scene]->OnSwitchOut();
	app.scenes[new_scene]->OnSwitchIn();

	app.current_scene = new_scene;

	/* The ECS needs to know the new scene. */
	app.registry.SetActiveScene(new_scene);
}

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
					logger->info("Toggling Full Screen...");
					window.ToggleFS();
				}

				if (evt.input == UserInput::TOGGLE_DEBUG_MODE)
				{
					logger->debug("Toggled Debug Mode.");
					debugModeEnabled = !debugModeEnabled;
				}
			}
		}

		/* Check if a Scene switch is wanted. */
		if (queued_scene != Scene::INVALID_HANDLE)
		{
			SwitchScene(queued_scene);
			queued_scene = Scene::INVALID_HANDLE;
		}

		scenes[current_scene]->OnUpdate();

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

		scenes[current_scene]->OnRender();

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

	/* At the end, write the config. */
	configManager.Set<int32_t>("windowWidth", window.GetWindowWidth());
	configManager.Set<int32_t>("windowHeight", window.GetWindowHeight());
	configManager.SaveConfig("mupfel.ini");

	logger->info("Wrote config to mupfel.ini.");
}

ImageManager& Mupfel::Application::GetCurrentImageManager() { return Get().image_manager; }
