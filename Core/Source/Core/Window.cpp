#include "Window.h"
#include "raylib.h"

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
	InitWindow(spec.width, spec.height, spec.title.c_str());
	SetTargetFPS(spec.target_fps);

	return true;
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
