#include "MovementSystem.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"
#include "ECS/Registry.h"
#include "Core/Profiler.h"
#include <algorithm>
#include <memory>
#include <iostream>

using namespace Mupfel;

/**
 * @brief Component signature mask describing which components the system requires (Transform + Velocity).
 */
static const Entity::Signature wanted_comp_sig = Registry::ComponentSignature<Mupfel::Transform, Mupfel::Movement>();

MovementSystem::MovementSystem()
{
}

MovementSystem::~MovementSystem()
{
}

void Mupfel::MovementSystem::Init()
{
	SetEventCallbacks();
}

void MovementSystem::DeInit()
{
}

void MovementSystem::Update(double elapsedTime)
{
	Move(elapsedTime);
}

void Mupfel::MovementSystem::Move(double elapsedTime)
{
	// implement cpu based move
}

void Mupfel::MovementSystem::SetEventCallbacks()
{
	Application::GetCurrentEventSystem().RegisterListener<ComponentAddedEvent>(
		[](const ComponentAddedEvent& event)
		{
			Entity::Signature test;
			test.set(event.comp_id);
			/* Check if even care about the entity */
			if ((test & wanted_comp_sig) == 0)
			{
				return;
			}

			if ((event.sig & wanted_comp_sig) != wanted_comp_sig)
			{
				/* Entity does not have Transform + Velocity */
				return;
			}
			// add to active entities
		}
	);

	Application::GetCurrentEventSystem().RegisterListener<ComponentRemovedEvent>(
		[](const ComponentRemovedEvent& event)
		{
			Entity::Signature test;
			test.set(event.comp_id);
			/* Check if even care about the entity */
			if ((test & wanted_comp_sig) == 0)
			{
				return;
			}

			Entity::Signature transform_sig;
			transform_sig.set(ComponentIndex::Index<Mupfel::Transform>());

			Entity::Signature velocity_sig;
			velocity_sig.set(ComponentIndex::Index<Mupfel::Movement>());

			uint32_t has_transform_component = (event.sig & transform_sig) != 0 ? 1 : 0;
			uint32_t has_velocity_component = (event.sig & velocity_sig) != 0 ? 1 : 0;

			uint32_t comp_info = has_transform_component + has_velocity_component;

			/*
				The remove event is always issued before the removal of the component,
				so we check if the entity currently has both of the components.
			*/
			if (comp_info != 2)
			{
				return;
			}

			// remove from active entities
		}
	);
}
