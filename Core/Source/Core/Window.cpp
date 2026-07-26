#include "Window.h"
#include <GLFW/glfw3.h>
#include "Core/Profiler.h"

using namespace Mupfel;

void Mupfel::Window::WaitEvents() const
{
	glfwWaitEvents();
}

GLFWwindow* Mupfel::Window::GetGLFWHandle() const
{
	return window;
}

void Mupfel::Window::GetFramebufferSize(int32_t& width, int32_t& height) const
{
	glfwGetFramebufferSize(window, &width, &height);
}

Mupfel::Window::Window()
{
}

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

bool Window::ShouldClose()
{
	return glfwWindowShouldClose(window);
}

void Mupfel::Window::PollEvents() const
{
	glfwPollEvents();
}

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

	return true;
}

void Mupfel::Window::ToggleFS()
{
	

	if (is_currently_fullscreen)
	{
		// TODO: set windowed size
	}
	else {
		// TODO: set fullscreen
	}
	is_currently_fullscreen = !is_currently_fullscreen;

}