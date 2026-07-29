#include "CollisionSystem.h"
#include <algorithm>
#include <cassert>
#include <array>
#include <thread>
#include "Core/Application.h"
#include "CollisionProcessor.h"
#include "ECS/Registry.h"

/* Needed Component types for collision detection/resolution */
#include "ECS/Components/Collider.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"

#include <iostream>

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
void Mupfel::CollisionSystem::SetCellSizePow(uint32_t cell_size_pow)
{
	collision_grid.SetCellSizePow(cell_size_pow);
}

void Mupfel::CollisionSystem::SetNumCells(uint32_t num_cells_x, uint32_t num_cells_y)
{
	collision_grid.SetNumCells(num_cells_x, num_cells_y);
}

void CollisionSystem::Init()
{
	/* Init the Collision Grid */
	collision_grid.Init();

	SetCallbacks();

	/* Register the Collision Resolvers */
	CollisionProcessor::RegisterCollisionResolver(ShapeType::Circle, ShapeType::Circle, CollisionProcessor::CircleCircle);
	CollisionProcessor::RegisterCollisionResolver(ShapeType::Circle, ShapeType::AABB, CollisionProcessor::CircleAABB);
	CollisionProcessor::RegisterCollisionResolver(ShapeType::AABB, ShapeType::AABB, CollisionProcessor::AABBAABB);
}

void CollisionSystem::Update()
{
	ClearBuffers();
	UpdateCellCount();
	FillCellEntityArray();
	NarrowPhase();
	CheckCollisions();
}


uint32_t Mupfel::CollisionSystem::WorldtoCell(Coordinate<uint32_t> c)
{
	uint32_t cell_x = c.x >> collision_grid.GetCellSizePow();
	uint32_t cell_y = c.y >> collision_grid.GetCellSizePow();

	cell_x = std::min(cell_x, collision_grid.GetNumCellsX() - 1);
	cell_y = std::min(cell_y, collision_grid.GetNumCellsY() - 1);

	return cell_y * collision_grid.GetNumCellsX() + cell_x;
}

void Mupfel::CollisionSystem::UpdateCellCount()
{
}

void Mupfel::CollisionSystem::FillCellEntityArray()
{
}

void Mupfel::CollisionSystem::NarrowPhase()
{
}

void Mupfel::CollisionSystem::CheckCollisions()
{
#if 0
	uint32_t num_colliding = num_colliding_entities->operator[](0);
	if (num_colliding > 0)
	{
		/* Iterate through the colliding entities */
		std::cout << "Num Collisions: " << num_colliding << std::endl;
		for (uint32_t i = 0; i < num_colliding; i++)
		{

			Entity a = colliding_entities->operator[](i).entity_a;
			Entity b = colliding_entities->operator[](i).entity_b;

			/* The CollisionProcessor handles Detection and Resolution of Entities */
			CollisionProcessor::DetectAndResolve(a, b);
		}
		
	}
#endif
}

void Mupfel::CollisionSystem::ClearBuffers()
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
