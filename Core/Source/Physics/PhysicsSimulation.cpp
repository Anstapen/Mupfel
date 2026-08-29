#include "PhysicsSimulation.h"
#include "Core/Profiler.h"
#include "ECS/Registry.h"

using namespace Mupfel;

PhysicsSimulation::PhysicsSimulation(Registry& in_reg, EventSystem& in_evt_system)
	: reg(in_reg), evt_system(in_evt_system), time_multi(1.0f), single_step(false)
{
	collision_system = std::make_unique<CollisionSystem>(reg, evt_system);
}

void PhysicsSimulation::Init()
{
	collision_system->Init();
}

void PhysicsSimulation::DeInit()
{
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

void Mupfel::PhysicsSimulation::SceneSwitched(SceneHandle new_scene, float grav_x, float grav_y)
{
	collision_system->SceneSwitched(new_scene, grav_x, grav_y);
}

void Mupfel::PhysicsSimulation::SetTimeMultiplier(double multi) { time_multi = multi; }

void Mupfel::PhysicsSimulation::ToggleSingleStep() { single_step = !single_step; }

void Mupfel::PhysicsSimulation::Step()
{
	/* When single stepping we use a fixed value of 1ms */
	collision_system->Step(0.001f, subSteps);
}

void Mupfel::PhysicsSimulation::SetTransform(Entity e, Transform t)
{
	collision_system->SetTransform(e, t);
}

void Mupfel::PhysicsSimulation::SetMovement(Entity e, float vel_x, float vel_y, float vel_ang)
{
	collision_system->SetMovement(e, vel_x, vel_y, vel_ang);
}

void Mupfel::PhysicsSimulation::GetContacts(Entity e, std::vector<ContactData>& buffer)
{
	collision_system->GetContacts(e, buffer);
}
