#include "PhysicsSimulation.h"
#include "Core/Profiler.h"
#include "ECS/Registry.h"
#include "MovementSystem.h"

using namespace Mupfel;

PhysicsSimulation::PhysicsSimulation(Registry& in_reg, EventSystem& in_evt_system)
	: reg(in_reg), evt_system(in_evt_system), time_multi(1.0f), single_step(false)
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
	collision_system->DeInit();
}

void PhysicsSimulation::Update(double elapsedTime)
{
	if (single_step)
	{
		return;
	}

	simAccumulator = std::min(simAccumulator + elapsedTime * time_multi, maxAccumulator);

	while (simAccumulator >= simDelta)
	{
		ProfilingSample prof("Box2D Step");
		collision_system->Step(simDelta, subSteps);
		simAccumulator -= simDelta;
	}

	collision_system->SyncTransforms();
	collision_system->DispatchEvents();
}

void Mupfel::PhysicsSimulation::SceneSwitched(SceneHandle new_scene) { collision_system->SceneSwitched(new_scene); }

void Mupfel::PhysicsSimulation::SetTimeMultiplier(double multi) { time_multi = multi; }

void Mupfel::PhysicsSimulation::ToggleSingleStep() { single_step = !single_step; }

void Mupfel::PhysicsSimulation::Step()
{
	/* When single stepping we use a fixed value of 1ms */
	MovementSystem::Update(0.001f);
	collision_system->Step(0.001f, subSteps);
}
