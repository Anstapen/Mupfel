#pragma once
#include <memory>
#include <cstdint>
#include "CollisionSystem.h"
#include "Core/EventSystem.h"
#include "Core/Scene.h"

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
		void SceneSwitched(SceneHandle new_scene, float grav_x, float grav_y);
		void SetTimeMultiplier(double multi);
		void ToggleSingleStep();
		void Step();

		void SetTransform(Entity e, Transform t);
		void SetMovement(Entity e, float vel_x, float vel_y, float vel_ang);
		void GetContacts(Entity e, std::vector<ContactData>& buffer);

	private:
		double time_multi;
		bool single_step;
		/** The simulation timestep (fixed at 60Hz). */
		static constexpr double simDelta = 1.0 / 60.0;
		static constexpr uint32_t subSteps = 4;
		static constexpr uint32_t maxStepsPerFrame = 2;
		static constexpr double	  maxAccumulator = maxStepsPerFrame * simDelta;

		/** Accumulator for the simulation delta. */
		double							 simAccumulator = 0.0f;
		Registry& reg;
		EventSystem& evt_system;
		std::unique_ptr<CollisionSystem> collision_system;
	};
}


