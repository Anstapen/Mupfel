#include "CollisionDetector.h"
#include <cassert>
#include "Core/Application.h"
#include <cmath>

/* Needed Components */
#include "ECS/Components/Collider.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"

using namespace Mupfel;

CollisionDetector::PossibleCollision Mupfel::CollisionDetector::Colliding(Entity a, Entity b)
{
	
	Registry& registry = Application::GetCurrentRegistry();

	assert(registry.HasComponent<Collider>(a));
	assert(registry.HasComponent<Collider>(b));
	assert(registry.HasComponent<Transform>(b));
	assert(registry.HasComponent<Transform>(a));
	assert(registry.HasComponent<Transform>(b));
	assert(registry.HasComponent<Transform>(a));

	Collider col_a = registry.GetComponent<Collider>(a);
	Collider col_b = registry.GetComponent<Collider>(b);

	PossibleCollision collison;

	/* Pretty ugly for now */

	/* Circle - Circle */
	if (col_a.info.type == ShapeType::Circle && col_b.info.type == ShapeType::Circle)
	{
		collison = CircleCircle(a, b);
	} else
	/* AABB - AABB */
	if (col_a.info.type == ShapeType::AABB && col_b.info.type == ShapeType::AABB)
	{
		collison = AABBAABB(a, b);
	} else
	/* Circle - AABB */
	if (col_a.info.type == ShapeType::AABB && col_b.info.type == ShapeType::Circle ||
		col_a.info.type == ShapeType::Circle && col_b.info.type == ShapeType::AABB)
	{
		collison = CircleAABB(a, b);
	}
	else
	{
		assert(0);
	}

	return collison;
}

CollisionDetector::PossibleCollision Mupfel::CollisionDetector::CircleCircle(Entity a, Entity b)
{
	/* First, collect all the components that we need */
	Registry& registry = Application::GetCurrentRegistry();

	assert(registry.HasComponent<Transform>(a));
	assert(registry.HasComponent<Transform>(b));
	assert(registry.HasComponent<Collider>(a));
	assert(registry.HasComponent<Collider>(b));

	Collider& collider_a = registry.GetComponent<Collider>(a);
	Collider& collider_b = registry.GetComponent<Collider>(b);

	Transform& transform_a = registry.GetComponent<Transform>(a);
	Transform& transform_b = registry.GetComponent<Transform>(b);

	/* We expect two Colliders of type Circle in this function! */
	assert(collider_a.info.type == ShapeType::Circle);
	assert(collider_b.info.type == ShapeType::Circle);

	float distance_x = transform_b.pos_x - transform_a.pos_x;
	float distance_y = transform_b.pos_y - transform_a.pos_y;

	float distance_squared = distance_x * distance_x + distance_y * distance_y;

	float radii_added = collider_a.GetCircle() + collider_b.GetCircle();

	float radii_squared = radii_added * radii_added;

	/* If the distance squared is larger than the radii squared, we DO NOT have a collision */
	if (distance_squared >= radii_squared)
	{
		return std::nullopt;
	}

	/* Catch the case of distance_squared being 0, that means the two entities are exactly on top of each other */
	if (distance_squared < 1e-6f)
	{
		return Result(glm::vec2(1.0f, 0.0f), radii_added);
	}

	/* Now we need the actual distance, so there is no way around sqrt... */
	float distance = sqrt(distance_squared);

	float penetration = radii_added - distance;

	glm::vec2 normal(distance_x / distance, distance_y / distance);

	return Result(normal, penetration);
}

CollisionDetector::PossibleCollision Mupfel::CollisionDetector::CircleAABB(Entity a, Entity b)
{
	return std::nullopt;
}

CollisionDetector::PossibleCollision Mupfel::CollisionDetector::AABBAABB(Entity a, Entity b)
{
	return std::nullopt;
}
