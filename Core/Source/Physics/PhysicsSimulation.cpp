#include "PhysicsSimulation.h"
#include "Core/Profiler.h"


using namespace Mupfel;

PhysicsSimulation::PhysicsSimulation(Registry& in_reg,EventSystem& in_evt_system) :
	reg(in_reg),
	evt_system(in_evt_system)
{

	movement_system = std::make_unique<MovementSystem>(1000);

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
	{
		ProfilingSample prof("Movement Update");
		movement_system->Update(elapsedTime);
	}
	{
		ProfilingSample prof("Collision Update");
		collision_system->Update();
	}
	
	
}
