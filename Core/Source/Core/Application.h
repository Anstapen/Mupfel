#pragma once

#include <string>
#include "Window.h"
#include "Layer.h"
#include <memory>
#include <vector>
#include "EventSystem.h"
#include "InputManager.h"
#include "ECS/Registry.h"
#include "Debug/DebugLayer.h"
#include "ThreadPool.h"
#include "Physics/PhysicsSimulation.h"

namespace Mupfel {

	/**
	 * @brief Defines the specification parameters used to initialize the Application.
	 *
	 * This structure provides all information needed for application setup,
	 * including the application name and window configuration.
	 */
	struct ApplicationSpecification {
		/**
		 * @brief The name of the application (used as window title).
		 */
		std::string name;

		/**
		 * @brief The configuration of the main window (size, title, vSync, etc.).
		 */
		WindowSpecification windowSpec;
	};

	/**
	 * @brief The main entry point of the Mupfel Engine.
	 *
	 * The Application class represents the central runtime controller
	 * of the entire engine. It manages window creation, event handling,
	 * rendering, physics simulation, input, debug overlay, and layer management.
	 *
	 * Only one instance of Application exists (Singleton pattern).
	 */
	class Application
	{
		friend class DebugLayer;

	public:
		/**
		 * @brief Retrieves the global instance of the Application.
		 * @return Reference to the singleton Application object.
		 */
		static Application& Get();

		/**
		 * @brief Destructor. Cleans up engine systems and resources.
		 */
		~Application();

		/**
		 * @brief Initializes the engine subsystems and creates the main window.
		 * @param spec The specification used for initialization.
		 * @return True if initialization succeeded.
		 */
		bool Init(const ApplicationSpecification& spec);

		/**
		 * @brief Starts the main application loop.
		 *
		 * This function runs the engine until the window is closed or
		 * Application::Stop() is called.
		 */
		void Run();

		/**
		 * @brief Stops the main application loop.
		 */
		void Stop();

		/**
		 * @brief Returns the current time in seconds since startup.
		 */
		static double GetCurrentTime();

		/**
		 * @brief Marks the beginning of a new frame for frame-time measurement.
		 *
		 * Should be called at the start of each frame.
		 */
		static void StartFrameTime();

		/**
		 * @brief Marks the end of a frame and optionally enforces a frame cap.
		 *
		 * Calculates frame time and applies frame limiting (e.g., 144 Hz).
		 */
		static void EndFrameTime();

		/**
		 * @brief Returns the time delta (in seconds) of the last rendered frame.
		 */
		static float GetLastFrameTime();

		/**
		 * @brief Returns a random integer between @p min and @p max.
		 */
		static int GetRandomNumber(int min, int max);

		/**
		 * @brief Returns the current width of the render surface in pixels.
		 */
		static int GetCurrentRenderWidth();

		/**
		 * @brief Returns the current height of the render surface in pixels.
		 */
		static int GetCurrentRenderHeight();

		/**
		 * @brief Returns whether the Debug Mode is currently enabled.
		 */
		static bool isDebugModeEnabled();

		/**
		 * @brief Provides access to the global Event System.
		 */
		static EventSystem& GetCurrentEventSystem();

		/**
		 * @brief Provides access to the global Input Manager.
		 */
		static InputManager& GetCurrentInputManager();

		/**
		 * @brief Provides access to the global ECS Registry.
		 */
		static Registry& GetCurrentRegistry();

		/**
		 * @brief Provides access to the global Thread Pool.
		 */
		static ThreadPool& GetCurrentThreadPool();

		/**
		 * @brief Pushes a new layer onto the application’s layer stack.
		 *
		 * Layers are used for modular engine and game logic that can
		 * respond to updates and rendering calls.
		 *
		 * @tparam TLayer A class derived from Layer.
		 */
		template<typename TLayer>
			requires(std::is_base_of_v<Layer, TLayer>)
		void PushLayer()
		{
			layerStack.push_back(std::make_unique<TLayer>());
			layerStack.back().get()->OnInit();
		}

	private:
		/**
		 * @brief Private constructor (Singleton pattern).
		 */
		Application();

		/**
		 * @brief Frees resources and deinitializes subsystems.
		 */
		void DeInit();

	private:
		/** @brief The startup specification of the application. */
		ApplicationSpecification spec;

		/** @brief Reference to the Window singleton. */
		Window& window;

		/** @brief Indicates whether the main loop is currently running. */
		bool running = false;

		/** @brief Enables or disables debug overlay rendering. */
		bool debugModeEnabled = false;

		/** @brief Stack of all active layers, managed by the application. */
		std::vector<std::unique_ptr<Layer>> layerStack;

		/** @brief Global event management system for engine and user events. */
		EventSystem evt_system;

		/** @brief Manages user input and input event mapping. */
		InputManager input_manager;

		/** @brief The ECS Registry that holds all entities and components. */
		Registry registry;

		/** @brief The global physics simulation system. */
		PhysicsSimulation physics;

		/** @brief Thread pool for multi-threaded jobs (e.g., physics, AI). */
		ThreadPool thread_pool;

		/** @brief The debug rendering layer used for profiling and visualization. */
		DebugLayer debug_layer;

		/** @brief Timestamp of the current frame’s start time. */
		double start_time = 0.0;

		/** @brief Duration of the most recently completed frame (in seconds). */
		double last_frame_time = 0.0;
	};
}



