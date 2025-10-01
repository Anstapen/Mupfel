#pragma once

#include <string>
#include <cstdint>

namespace Mupfel {

	struct WindowSpecification {
		std::string title;
		uint32_t width = 1280;
		uint32_t height = 720;
		bool isResizeable = true;
		bool vSync = true;
		uint32_t target_fps = 60;
	};

	class Application;

	class Window
	{
	friend Application;
	private:
		virtual ~Window();

		static Window& GetInstance();

		bool Init(const WindowSpecification& spec);

		void StartFrame();

		void EndFrame();

		bool ShouldClose();
	private:
		Window();
	private:
		WindowSpecification spec;
	};
}


