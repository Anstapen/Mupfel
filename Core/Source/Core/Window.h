#pragma once

#include <string>
#include <cstdint>

namespace Mupfel {

	/**
	 * @brief Describes the configuration parameters used to create a Window.
	 *
	 * The WindowSpecification structure defines the basic settings
	 * for initializing the main application window, including its
	 * title, resolution, frame rate, and behavior (e.g., VSync, resizing).
	 */
	struct WindowSpecification {
		/** @brief Title of the application window. */
		std::string title;

		/** @brief Initial width of the window in pixels. */
		uint32_t width = 2000;

		/** @brief Initial height of the window in pixels. */
		uint32_t height = 1000;

		/** @brief Determines whether the window can be resized by the user. */
		bool isResizeable = true;

		/** @brief Enables or disables vertical synchronization (VSync). */
		bool vSync = true;

		/** @brief Target frame rate (frames per second). */
		uint32_t target_fps = 60;
	};

	class Application;

	/**
	 * @brief Represents the main application window.
	 *
	 * The Window class provides a simple interface for window creation,
	 * resizing, fullscreen toggling, and per-frame rendering control.
	 *
	 * It is implemented as a singleton to ensure that only one window
	 * exists throughout the application's lifetime. The Application class
	 * has friend access to allow it to manage the window directly.
	 *
	 * Internally, this class wraps raylib’s window and drawing functions
	 * and integrates with the engine’s profiling and frame management systems.
	 */
	class Window
	{
		friend Application;

	private:
		/**
		 * @brief Destructor.
		 *
		 * Automatically closes the window and cleans up raylib resources
		 * when the application shuts down.
		 */
		virtual ~Window();

		/**
		 * @brief Returns the global Window instance.
		 * @return Reference to the singleton Window object.
		 *
		 * This ensures only one window is created and reused
		 * throughout the engine’s lifetime.
		 */
		static Window& GetInstance();

		/**
		 * @brief Initializes the window using the provided specification.
		 * @param spec The configuration data defining window size, title, and behavior.
		 * @return True if the window was successfully created.
		 *
		 * This function initializes the raylib window, sets its flags,
		 * and stores the provided specification internally.
		 */
		bool Init(const WindowSpecification& spec);

		/**
		 * @brief Resizes the window to the given dimensions.
		 * @param width New width of the window (in pixels).
		 * @param height New height of the window (in pixels).
		 *
		 * Updates both raylib’s internal window state and
		 * the engine’s local width/height tracking.
		 */
		void resize(int width, int height);

		/**
		 * @brief Toggles between fullscreen and windowed mode.
		 *
		 * This function switches the current display mode while
		 * preserving the previous window dimensions for restoration.
		 */
		void ToggleFS();

		/**
		 * @brief Prepares the frame for rendering.
		 *
		 * Begins the raylib drawing process and clears the window background.
		 * Should be called once per frame before rendering any content.
		 */
		void StartFrame();

		/**
		 * @brief Finalizes the current frame and presents it to the display.
		 *
		 * Ends the raylib drawing context and swaps the back buffer to the screen.
		 */
		void EndFrame();

		/**
		 * @brief Checks whether the window should close.
		 * @return True if the user requested to close the window.
		 *
		 * This typically responds to OS-level close events (e.g., pressing the X button
		 * or Alt+F4) and is used by the main loop to determine exit conditions.
		 */
		bool ShouldClose();

	private:
		/**
		 * @brief Private constructor to enforce singleton behavior.
		 *
		 * The Window instance can only be accessed via GetInstance().
		 */
		Window();

	private:
		/** @brief The current window configuration specification. */
		WindowSpecification spec;

		/** @brief Indicates whether the window is currently in fullscreen mode. */
		bool is_currently_fullscreen = false;

		/** @brief The current window width (used for restoring size when exiting fullscreen). */
		int current_width = 0;

		/** @brief The current window height (used for restoring size when exiting fullscreen). */
		int current_height = 0;
	};
}


