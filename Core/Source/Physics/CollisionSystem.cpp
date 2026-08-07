#include "CollisionSystem.h"
#include <algorithm>
#include <cassert>
#include <array>
#include <thread>
#include "Core/Application.h"

/* Needed Component types for collision detection/resolution */
#include "ECS/Components/Collider.h"
#include "ECS/Components/Transform.h"

using namespace Mupfel;

/**
 * @brief Component signature mask describing which components the system requires (Transform + Velocity).
 */
static const Entity::Signature wanted_comp_sig = Registry::ComponentSignature<Mupfel::Transform, Mupfel::Collider>();


static const uint32_t max_colliding_entities = 20000;


Mupfel::CollisionSystem::CollisionSystem(Registry& reg, EventSystem& evt_sys) :
	registry(reg),
	evt_system(evt_sys)
{
}


void CollisionSystem::Init()
{
}

void CollisionSystem::Update()
{
}



void Mupfel::CollisionSystem::SetCallbacks()
{
	Application::GetCurrentEventSystem().RegisterListener<ComponentAddedEvent>(
		[this](const ComponentAddedEvent& event)
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
				return;
			}

			// add to active entities
		}
	);

	Application::GetCurrentEventSystem().RegisterListener<ComponentRemovedEvent>(
		[this](const ComponentRemovedEvent& event)
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

			Entity::Signature spatial_info_sig;
			spatial_info_sig.set(ComponentIndex::Index<Mupfel::Collider>());

			uint32_t has_transform_component = (event.sig & transform_sig) != 0 ? 1 : 0;
			uint32_t has_spatial_component = (event.sig & spatial_info_sig) != 0 ? 1 : 0;

			uint32_t comp_info = has_transform_component + has_spatial_component;

			/*
				The remove event is always issued before the removal of the component,
				so we check if the entity currently has both of the components.
			*/
			if (comp_info != 2)
			{
				return;
			}

			// remove the entity
		}
	);
}
