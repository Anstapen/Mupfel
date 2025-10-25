#pragma once
#include <memory>
#include "CollisionSystem.h"
#include "MovementSystem.h"
#include "Core/ComputationStrategy.h"
#include "ECS/Registry.h"
#include "Core/EventSystem.h"

namespace Mupfel {

	/**
	 * @brief This is the class that is responsible for simulating several things:
	 * 
	 * 1. Entity movement (velocity, acceleration, position)
	 * 2. Entity collision (mass, shape)
	 */
	class PhysicsSimulation
	{
	public:
		PhysicsSimulation(Registry& in_reg, EventSystem& in_evt_system, ComputationStrategy in_strat = ComputationStrategy::CPU_SINGLE_THREADED);
		virtual ~PhysicsSimulation() = default;
		void Init();
		void DeInit();
		void Update(double elapsedTime);
		void ChangeComputationStrategy(ComputationStrategy new_strat);

	private:
		Registry& reg;
		EventSystem& evt_system;
		ComputationStrategy current_strat;
		std::unique_ptr<MovementSystem> movement_system;
		std::unique_ptr<CollisionSystem> collision_system;
	};
}


