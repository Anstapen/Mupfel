#include "Application.h"
#include "raylib.h"
#include <algorithm>
#include "Profiler.h"
#include "Renderer/Renderer.h"
#include <iostream>
#include "glad.h"


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

static void GLAPIENTRY
MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	if (severity <= GL_DEBUG_SEVERITY_NOTIFICATION)
	{
		return;
	}
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
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

	physics.Init();

	debug_layer.OnInit();

	Renderer::Init();
	

	// During init, enable debug output
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

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

void Mupfel::Application::StartFrameTime()
{
	Get().start_time = GetTime();
}

void Mupfel::Application::EndFrameTime()
{
	double current_time = GetTime();
	double frame_time = current_time - Get().start_time;

	double wait_time = (1.0f / 144.0f) - frame_time;

	if (wait_time > 0.0f)
	{
		WaitTime((float)wait_time);
		current_time = GetTime();
		frame_time = (float)(current_time - Get().start_time);
	}

	Get().last_frame_time = frame_time;
}

float Mupfel::Application::GetLastFrameTime()
{
	return Get().last_frame_time;
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


static GLuint gpuTimerQuery = 0;


void Application::Run()
{
	running = true;

	double lastTime = Application::GetCurrentTime();

	glGenQueries(1, &gpuTimerQuery);

	/* Main Loop */
	while (running)
	{
		if (window.ShouldClose())
		{
			Stop();
			break;
		}
		Application::StartFrameTime();
		ProfilingSample prof("Application::Run()");

		double currentTime = Application::GetCurrentTime();
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
			window.StartFrame();
			Renderer::Render();
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
			window.EndFrame();
			
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
}