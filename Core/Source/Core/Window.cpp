#include "Window.h"
#include "raylib.h"
#include "Core/Profiler.h"

using namespace Mupfel;

Mupfel::Window::Window()
{
}

Mupfel::Window::~Window()
{
	CloseWindow();
}

Window& Mupfel::Window::GetInstance()
{
	static Window window;
	return window;
}

bool Window::ShouldClose()
{
	return WindowShouldClose();
}

bool Window::Init(const WindowSpecification& spec)
{
	this->spec = spec;
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(spec.width, spec.height, spec.title.c_str());
	SetTargetFPS(0);

	current_height = spec.height;
	current_width = spec.width;

	return true;
}

void Mupfel::Window::resize(int width, int height)
{
	SetWindowSize(width, height);
	current_height = height;
	current_width = width;
}

void Mupfel::Window::ToggleFS()
{
	

	if (is_currently_fullscreen)
	{
		SetWindowSize(current_width, current_height);
	}
	else {
		SetWindowSize(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
	}
	ToggleFullscreen();
	is_currently_fullscreen = !is_currently_fullscreen;

}

void Mupfel::Window::StartFrame()
{
	BeginDrawing();
	ClearBackground(BLANK);
}

void Mupfel::Window::EndFrame()
{
	EndDrawing();
}
