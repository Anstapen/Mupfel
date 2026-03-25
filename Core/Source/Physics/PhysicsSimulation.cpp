#include "PhysicsSimulation.h"
#include "Core/Profiler.h"
#include "MovementSystem.h"


using namespace Mupfel;

PhysicsSimulation::PhysicsSimulation(Registry& in_reg,EventSystem& in_evt_system) :
	reg(in_reg),
	evt_system(in_evt_system),
	time_multi(1.0f),
	single_step(false)
{
	collision_system = std::make_unique<CollisionSystem>(reg, evt_system);
}

void PhysicsSimulation::Init()
{
	collision_system->Init();
	MovementSystem::Init();
}

void PhysicsSimulation::DeInit()
{
	MovementSystem::DeInit();
}

void PhysicsSimulation::Update(double elapsedTime)
{
	if (single_step)
	{
		return;
	}
	{
		ProfilingSample prof("Movement Update");
		MovementSystem::Update(elapsedTime * time_multi);
	}
	{
		ProfilingSample prof("Collision Update");
		collision_system->Update();
	}
	
	
}

void Mupfel::PhysicsSimulation::SetTimeMultiplier(double multi)
{
	time_multi = multi;
}

void Mupfel::PhysicsSimulation::ToggleSingleStep()
{
	single_step = !single_step;
}

void Mupfel::PhysicsSimulation::Step()
{
	/* When single stepping we use a fixed value of 100ms */
	MovementSystem::Update(0.001f);
	collision_system->Update();
}
