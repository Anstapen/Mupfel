#include "Application.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <timeapi.h>
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#endif

#include "Debug/DebugLayer.h"
#include "DefaultScene.h"
#include "ECS/Registry.h"
#include "Logger.h"
#include "Physics/PhysicsSimulation.h"
#include "Profiler.h"
#include "Renderer/AnimationSystem.h"
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

Camera& Mupfel::Application::GetCurrentSceneCamera() { return Get().scenes[Get().current_scene]->camera; }

KeyAction Mupfel::Application::GetKey(Key k) { return Get().window.GetKey(k); }

KeyAction Mupfel::Application::GetMouseButton(MouseButton b) { return Get().window.GetMouseButton(b); }

Application::Application()
	: window(Window::GetInstance()), evt_system(), input_manager(evt_system),
	  thread_pool(std::thread::hardware_concurrency()), registry(evt_system, thread_pool)
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

	if (!Window::GetInstance().Init(window_spec))
	{
		logger->error("Window Initialization failed!");
		return false;
	}

	renderer = std::make_unique<Renderer>();
	if (!renderer->Init(Window::GetInstance()))
	{
		logger->error("Renderer Initialization failed!");
		return false;
	}

	physics = std::make_unique<PhysicsSimulation>(registry, evt_system);
	physics->Init();

	debug_layer = std::make_unique<DebugLayer>();
	debug_layer->OnInit();

	animationSystem = std::make_unique<AnimationSystem>();

	if (!animationSystem->Init())
	{
		logger->error("Animation System Initialization failed!");
		return false;
	}

	/* Add Scene 0 */
	SceneHandle first_handle = CreateScene<DefaultScene>({"DefaultScene"});

	/* prevent unused variable warning in release builds */
	(void)first_handle;

	/* We should be the first ones to create a Scene! */
	assert(first_handle == 0);

	frame_count = 0;

#ifdef _WIN32
	timeBeginPeriod(1);
#endif

	return true;
}

void Application::Stop() { running = false; }

double Application::GetTime() { return std::chrono::duration<double>(Clock::now() - start_time).count(); }

void Mupfel::Application::StartFrameTime() { Get().start_frame_time = GetTime(); }

void Mupfel::Application::EndFrameTime()
{
	auto& app = Get();

	if (app.targetFPS > 0)
	{
		const double desired_frame_time = 1.0 / static_cast<double>(app.targetFPS);
		app.next_frame_deadline += desired_frame_time;

		double now = GetTime();

		if (now > app.next_frame_deadline)
		{
			/* We are behind the target FPS, do not sleep. */
			app.next_frame_deadline = now;
		}
		else
		{
			constexpr double spin_margin = 0.002;
			const double	 remaining = app.next_frame_deadline - now;

			/* We only wait for a percentage of the total waiting time, to minimize overshooting. */
			if (remaining > spin_margin)
			{
				WaitTime(remaining - spin_margin);
			}

			/* Precise wait for the rest. */
			while (GetTime() < app.next_frame_deadline)
			{
				std::this_thread::yield();
			}
		}
	}

	app.last_frame_time = GetTime() - app.start_frame_time;
}

void Mupfel::Application::WaitTime(double time) { std::this_thread::sleep_for(std::chrono::duration<double>(time)); }

float Mupfel::Application::GetLastFrameTime() { return static_cast<float>(Get().last_frame_time); }

void Mupfel::Application::SetTargetFPS(uint32_t target_fps) { Get().targetFPS = target_fps; }

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

void Mupfel::Application::SetTransform(Entity e, Transform t) { return Get().physics->SetTransform(e, t); }

void Mupfel::Application::SetMovement(Entity e, float vel_x, float vel_y, float vel_ang)
{
	return Get().physics->SetMovement(e, vel_x, vel_y, vel_ang);
}

bool Mupfel::Application::HasPhysicsEvents(Entity e) { return Get().physics->HasPhysicsEvents(e); }

Expected<ImageHandle> Mupfel::Application::LoadBasicImage(const std::string path)
{
	return Get().renderer->GetImageManager().Load(path);
}

Expected<ImageHandle> Mupfel::Application::LoadAnimatedImage(const std::string path, const ImageSpecification& spec)
{
	return Get().renderer->GetImageManager().LoadAnimated(path, spec);
}

Expected<std::vector<ImageHandle>>
Mupfel::Application::LoadSpriteSheetImages(const std::string path, const ImageSpecification& spec)
{
	return Get().renderer->GetImageManager().LoadSpriteSheet(path, spec);
}

ThreadPool& Mupfel::Application::GetCurrentThreadPool() { return Get().thread_pool; }

void Mupfel::Application::SetTimeScale(double time_scale) { Get().physics->SetTimeMultiplier(time_scale); }

void Mupfel::Application::TogglePhysicsSingleStep() { Get().physics->ToggleSingleStep(); }

void Mupfel::Application::PhysicsStep() { Get().physics->Step(); }

void Mupfel::Application::SwitchScene(SceneHandle new_scene)
{
	auto& app = Get();
	if (new_scene >= Scene::MAX_SCENES)
	{
		return;
	}

	if (new_scene == app.current_scene)
	{
		return;
	}

	if (!app.scenes[new_scene])
	{
		return;
	}

	if (app.current_scene < Scene::INVALID_HANDLE)
	{
		app.scenes[app.current_scene]->OnSwitchOut();
	}

	app.scenes[new_scene]->OnSwitchIn();

	app.current_scene = new_scene;

	/* The ECS and PhysicsSystem need to know the new scene. */
	app.registry.SetActiveScene(new_scene);
	app.physics->SceneSwitched(new_scene, app.scenes[new_scene]->gravity_x, app.scenes[new_scene]->gravity_y);
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
		ProfilingSample main_app("Application::Run()");

		double currentTime = Application::GetTime();
		double timestep = std::clamp<double>(currentTime - lastTime, 0.00001f, 0.1f);
		lastTime = currentTime;

		{
			ProfilingSample prof("Application::Run(): Check ");
			/* Check for Application related changes */
			if (input_manager.CheckUserInput(UserInput::WINDOW_FULLSCREEN, KeyAction::NONE))
			{
				logger->info("Toggling Full Screen...");
				window.ToggleFS();
			}

			if (input_manager.CheckUserInput(UserInput::TOGGLE_DEBUG_MODE, KeyAction::NONE))
			{
				logger->debug("Toggled Debug Mode.");
				debugModeEnabled = !debugModeEnabled;
			}
		}

		/* Check if a Scene switch is wanted. */
		if (queued_scene != Scene::INVALID_HANDLE)
		{
			SwitchScene(queued_scene);
			queued_scene = Scene::INVALID_HANDLE;
		}

		{
			ProfilingSample prof("Current Scene - OnUpdate ");
			scenes[current_scene]->OnUpdate(timestep);
		}

		{
			ProfilingSample prof("Layers - OnUpdate ");
			/* Update all layers */
			for (const std::unique_ptr<Layer>& layer : layerStack)
			{
				layer->OnUpdate(timestep);
			}
			debug_layer->OnUpdate(timestep);
		}

		{
			ProfilingSample prof("Physics Update");

			physics->Update(timestep);
		}

		{
			ProfilingSample prof("Animation Update");
			/* Update the Collision System */
			animationSystem->Update(timestep);
		}

		{
			ProfilingSample prof("Engine Renderer Begin");
			renderer->Begin(Window::GetInstance(), timestep);
		}

		{
			ProfilingSample prof("Current Scene - OnRender");
			scenes[current_scene]->OnRender();
		}

		{
			ProfilingSample prof("Layer Rendering");
			for (const std::unique_ptr<Layer>& layer : layerStack)
			{
				layer->OnRender();
			}
		}

		{
			ProfilingSample prof("DebugLayer");
			if (debugModeEnabled)
			{

				/* Make sure the debug Layer is rendered last */
				debug_layer->OnRender();
			}
		}

		{
			ProfilingSample prof("Engine Renderer End");
			renderer->End(Window::GetInstance(), timestep);
		}

		{
			ProfilingSample prof("Event System Update");
			/* Update the EventSystem */
			evt_system.Update();
		}

		Profiler::Clear();

		Application::EndFrameTime();
	}

	/* Exited Main Loop, clean everything up */
	DeInit();
}

void Application::DeInit()
{
#ifdef _WIN32
	timeEndPeriod(1);
#endif
	physics->DeInit();
	renderer->Shutdown();
	animationSystem->Shutdown();

	/* At the end, write the config. */
	configManager.Set<int32_t>("windowWidth", window.GetWindowWidth());
	configManager.Set<int32_t>("windowHeight", window.GetWindowHeight());
	configManager.SaveConfig("mupfel.ini");

	logger->info("Wrote config to mupfel.ini.");
}
