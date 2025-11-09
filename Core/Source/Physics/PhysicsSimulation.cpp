#include "PhysicsSimulation.h"
#include "Core/Profiler.h"
#include "MovementSystem.h"


using namespace Mupfel;

PhysicsSimulation::PhysicsSimulation(Registry& in_reg,EventSystem& in_evt_system) :
	reg(in_reg),
	evt_system(in_evt_system)
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
	{
		ProfilingSample prof("Movement Update");
		MovementSystem::Update(elapsedTime);
	}
	{
		ProfilingSample prof("Collision Update");
		collision_system->Update();
	}
	
	
}
