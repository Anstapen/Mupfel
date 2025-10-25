#pragma once

#include <string>
#include "Window.h"
#include "Layer.h"
#include <memory>
#include <vector>
#include "EventSystem.h"
#include "InputManager.h"
#include "ECS/Registry.h"
#include "Physics/CollisionSystem.h"
#include "Debug/DebugLayer.h"
#include "ThreadPool.h"
#include "Physics/PhysicsSimulation.h"

namespace Mupfel {

	struct ApplicationSpecification {
		std::string name;
		WindowSpecification windowSpec;
	};

	class Application
	{
		friend class DebugLayer;
	public:
		static Application& Get();
		
		~Application();

		bool Init(const ApplicationSpecification& spec);
		void Run();
		void Stop();

		static double GetCurrentTime();
		static float GetLastFrameTime();
		static int GetRandomNumber(int min, int max);
		static int GetCurrentRenderWidth();
		static int GetCurrentRenderHeight();
		static bool isDebugModeEnabled();
		static EventSystem& GetCurrentEventSystem();
		static InputManager& GetCurrentInputManager();
		static Registry& GetCurrentRegistry();
		static ThreadPool& GetCurrentThreadPool();

		template<typename TLayer>
			requires(std::is_base_of_v<Layer, TLayer>)
		void PushLayer()
		{
			layerStack.push_back(std::make_unique<TLayer>());
			layerStack.back().get()->OnInit();
		}
	private:
		Application();
		void DeInit();
	private:
		ApplicationSpecification spec;
		Window& window;
		bool running = false;
		bool debugModeEnabled = false;
		std::vector<std::unique_ptr<Layer>> layerStack;
		EventSystem evt_system;
		InputManager input_manager;
		Registry registry;
		PhysicsSimulation physics;
		ThreadPool thread_pool;
		DebugLayer debug_layer;
	};
}



