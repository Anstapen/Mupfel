#include "Window.h"
#include "Application.h"
#include "Core/Profiler.h"
#include <GLFW/glfw3.h>
#include <algorithm>

using namespace Mupfel;

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	KeyAction keyaction;
	switch (action)
	{
	case GLFW_PRESS:
		keyaction = KeyAction::PRESSED;
		break;
	case GLFW_RELEASE:
		keyaction = KeyAction::RELEASED;
		break;
	case GLFW_REPEAT:
		keyaction = KeyAction::REPEATED;
		break;
	default:
		keyaction = KeyAction::PRESSED;
		break;
	}
	Application::GetCurrentInputManager().KeyPressed(static_cast<Key>(key), keyaction);
}

void Window::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	Application::GetCurrentInputManager().UpdateCursor(xpos, ypos);
}

void Mupfel::Window::WaitEvents() const { glfwWaitEvents(); }

GLFWwindow* Mupfel::Window::GetGLFWHandle() const { return window; }

void Mupfel::Window::GetFramebufferSize(int32_t& width, int32_t& height) const
{
	glfwGetFramebufferSize(window, &width, &height);
}

Mupfel::Window::Window() {}

Mupfel::Window::~Window()
{
	if (window)
	{
		glfwDestroyWindow(window);
	}
}

Window& Mupfel::Window::GetInstance()
{
	static Window window;
	return window;
}

bool Window::ShouldClose() { return glfwWindowShouldClose(window); }

bool Mupfel::Window::IsMinimized() const
{
	int32_t width, height;
	GetFramebufferSize(width, height);

	return (width == 0) || (height == 0);
}

int32_t Mupfel::Window::GetWindowWidth() const { return current_width; }

int32_t Mupfel::Window::GetWindowHeight() const { return current_height; }

void Mupfel::Window::PollEvents() const { glfwPollEvents(); }

bool Window::Init(const WindowSpecification& spec)
{
	this->spec = spec;

	/* Zero dimensions in the spec mean that we should search for a good default. */
	const std::string window_name = (spec.title.empty()) ? "Mupfel" : spec.title;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

	GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();

	const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);

	this->window = glfwCreateWindow(mode->width, mode->height, window_name.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	if (!this->window)
	{
		return false;
	}

	glfwGetWindowPos(window, &current_pos_x, &current_pos_y);
	glfwSetWindowPos(window, 0, current_pos_y);
	glfwGetWindowSize(window, &current_width, &current_height);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	return true;
}

void Mupfel::Window::ToggleFS()
{

	if (is_currently_fullscreen)
	{
		glfwSetWindowMonitor(window, NULL, current_pos_x, current_pos_y, current_width, current_height, 0);
	}
	else
	{
		// TODO: set fullscreen
		glfwGetWindowPos(window, &current_pos_x, &current_pos_y);
		glfwGetWindowSize(window, &current_width, &current_height);
		GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();

		const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);

		glfwSetWindowMonitor(window, primary_monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	}
	is_currently_fullscreen = !is_currently_fullscreen;
}