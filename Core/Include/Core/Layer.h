#pragma once

namespace Mupfel {

	/**
	 * @brief Base class for all application and engine layers.
	 *
	 * The Layer class represents a modular section of the engine or application logic.
	 * Layers are used to separate different systems or rendering stages — for example,
	 * gameplay layers, UI layers, debug overlays, or post-processing passes.
	 *
	 * Each Layer can respond to events, initialize its state, update every frame,
	 * and perform rendering.
	 *
	 * Layers are managed by the Application and are executed in the order
	 * they were added to the layer stack.
	 */
	class Layer
	{
	public:
		/**
		 * @brief Default constructor.
		 */
		Layer() = default;

		/**
		 * @brief Virtual destructor.
		 *
		 * Ensures proper cleanup of derived layers when destroyed
		 * through a base pointer.
		 */
		virtual ~Layer() = default;

		/**
		 * @brief Called when an engine or user event occurs.
		 *
		 * Derived classes can override this function to handle
		 * input, physics, or custom application events.
		 */
		virtual void OnEvent() {}

		/**
		 * @brief Called once when the layer is first initialized.
		 *
		 * This function is typically used to allocate resources,
		 * load textures, compile shaders, or initialize scene data.
		 */
		virtual void OnInit() {}

		/**
		 * @brief Called once per frame to update the layer’s logic.
		 * @param timestep The time (in seconds) since the last frame.
		 *
		 * Use this to update game logic, animations, or other
		 * time-dependent behavior.
		 */
		virtual void OnUpdate(double timestep) {}

		/**
		 * @brief Called once per frame to render the layer’s contents.
		 *
		 * Use this to perform all drawing commands related to this layer,
		 * such as rendering meshes, UI elements, or debug information.
		 */
		virtual void OnRender() {}
	};

}

