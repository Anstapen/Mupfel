#pragma once

namespace Mupfel {

	/**
	 * @brief Implements the Movement System of Mupfel.
	 *
	 * The MovementSystem is a subsystem within the Physics pipeline.
	 * It is responsible for updating entity positions based on their
	 * Velocity and Transform components using GPU compute shaders.
	 */
	class MovementSystem {
	public:
		/**
		 * @brief Initializes all GPU resources and loads compute shaders.
		 */
		static void Init();

		/**
		 * @brief Frees resources used by the Movement System.
		 */
		static void DeInit();

		/**
		 * @brief Updates the Movement System once per frame.
		 * @param elapsedTime Time in seconds since the last frame.
		 */
		static void Update(double elapsedTime);

	private:
		/**
		 * @brief Constructor is private since this class is static.
		 */
		MovementSystem();

		/**
		 * @brief Destructor.
		 */
		virtual ~MovementSystem();

		/**
		 * @brief Performs a GPU-side join between Transform and Velocity
		 * component arrays to find active entities with both components.
		 */
		static void Join();

		/**
		 * @brief Executes the main movement compute shader to update entity positions.
		 * @param elapsedTime Delta time used to compute movement updates.
		 */
		static void Move(double elapsedTime);

		/**
		 * @brief Updates the GPU buffer that holds Shader Program parameters.
		 * @param elapsedTime Delta time for this frame.
		 */
		static void SetProgramParams(double elapsedTime);

		/**
		 * @brief Registers event listeners for component addition/removal events.
		 */
		static void SetEventCallbacks();
	};

}


