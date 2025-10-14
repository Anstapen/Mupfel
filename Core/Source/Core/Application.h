#pragma once

#include <string>
#include "Window.h"
#include "Layer.h"
#include <memory>
#include <vector>
#include "EventSystem.h"
#include "InputManager.h"
#include "ECS/Registry.h"

namespace Mupfel {

	struct ApplicationSpecification {
		std::string name = "Application";
		WindowSpecification windowSpec;
	};

	class Application
	{
	public:
		static Application& Get();
		
		~Application();

		bool Init(const ApplicationSpecification& spec);
		void Run();
		void Stop();

		static float GetCurrentTime();
		static EventSystem& GetCurrentEventSystem();
		static InputManager& GetCurrentInputManager();
		static Registry& GetCurrentRegistry();

		template<typename TLayer>
			requires(std::is_base_of_v<Layer, TLayer>)
		void PushLayer()
		{
			layerStack.push_back(std::make_unique<TLayer>());
			layerStack.back().get()->OnInit();
		}
	private:
		Application();
	private:
		ApplicationSpecification spec;
		Window& window;
		bool running = false;
		std::vector<std::unique_ptr<Layer>> layerStack;
		EventSystem evt_system;
		InputManager input_manager;
		Registry registry;
	};
}



