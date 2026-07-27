#include "Window.h"
#include "Application.h"
#include "Core/Profiler.h"
#include <GLFW/glfw3.h>

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

void Mupfel::Window::PollEvents() const { glfwPollEvents(); }

bool Window::Init(const WindowSpecification& spec)
{
	this->spec = spec;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	const std::string window_name("Vulkan Playground");
	uint32_t		  window_size_x = spec.width;
	uint32_t		  window_size_y = spec.height;
	this->window = glfwCreateWindow(window_size_x, window_size_y, window_name.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	if (!this->window)
	{
		return false;
	}

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	return true;
}

void Mupfel::Window::ToggleFS()
{

	if (is_currently_fullscreen)
	{
		// TODO: set windowed size
	}
	else
	{
		// TODO: set fullscreen
	}
	is_currently_fullscreen = !is_currently_fullscreen;
}