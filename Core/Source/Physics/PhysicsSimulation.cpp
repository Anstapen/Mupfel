#include "PhysicsSimulation.h"
#include "MovementSystemCPUST.h"
#include "MovementSystemCPUMT.h"
#include "MovementSystemGPU.h"

using namespace Mupfel;

PhysicsSimulation::PhysicsSimulation(Registry& in_reg,EventSystem& in_evt_system, ComputationStrategy in_strat) :
	reg(in_reg),
	current_strat(in_strat),
	evt_system(in_evt_system)
{
	/* Simple switch case is enough here, there will not be many cases */
	switch (current_strat)
	{
		case ComputationStrategy::CPU_SINGLE_THREADED:
			movement_system = std::make_unique<MovementSystemCPUST>(reg);
			break;
		case ComputationStrategy::CPU_MULTITHREADED:
			movement_system = std::make_unique<MovementSystemCPUMT>(reg);
			break;
		case ComputationStrategy::GPU:
			movement_system = std::make_unique<MovementSystemGPU>(reg);
			break;
		default:
			break;
	}

	collision_system = std::make_unique<CollisionSystem>(reg, evt_system);
}

void PhysicsSimulation::Init()
{
	collision_system->Init();
	movement_system->Init();
}

void PhysicsSimulation::DeInit()
{
	movement_system->DeInit();
}

void PhysicsSimulation::Update(double elapsedTime)
{
	movement_system->Update(elapsedTime);
	collision_system->Update();
}

void Mupfel::PhysicsSimulation::ChangeComputationStrategy(ComputationStrategy new_strat)
{
	if (current_strat == new_strat)
	{
		return;
	}

	switch (new_strat)
	{
	case ComputationStrategy::CPU_SINGLE_THREADED:
		movement_system = std::make_unique<MovementSystemCPUST>(reg);
		break;
	case ComputationStrategy::CPU_MULTITHREADED:
		movement_system = std::make_unique<MovementSystemCPUMT>(reg);
		break;
	case ComputationStrategy::GPU:
		movement_system = std::make_unique<MovementSystemGPU>(reg);
		break;
	default:
		break;
	}

	current_strat = new_strat;
}
