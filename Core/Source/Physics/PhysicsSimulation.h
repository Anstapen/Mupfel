#pragma once
#include <memory>
#include <cstdint>
#include "CollisionSystem.h"
#include "Core/EventSystem.h"

namespace Mupfel {

	class DebugLayer;
	class Registry;

	/**
	 * @brief This is the class that is responsible for simulating several things:
	 * 
	 * 1. Entity movement (velocity, acceleration, position)
	 * 2. Entity collision (mass, shape)
	 */
	class PhysicsSimulation
	{
	friend class DebugLayer;
	public:
		PhysicsSimulation(Registry& in_reg, EventSystem& in_evt_system);
		virtual ~PhysicsSimulation() = default;
		void Init();
		void DeInit();
		void Update(double elapsedTime);
		void SetTimeMultiplier(double multi);
		void ToggleSingleStep();
		void Step();

	private:
		double time_multi;
		bool single_step;
		/** The simulation timestep (fixed at 100Hz). */
		static constexpr double simDelta = 1.0f / 100.0f;
		static constexpr uint32_t subSteps = 4;
		static constexpr double	  maxAccumulator = 0.25f;

		/** Accumulator for the simulation delta. */
		double							 simAccumulator = 0.0f;
		Registry& reg;
		EventSystem& evt_system;
		std::unique_ptr<CollisionSystem> collision_system;
	};
}


