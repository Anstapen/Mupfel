#include "CollisionProcessor.h"
#include <cassert>
#include "Core/Application.h"
#include <cmath>

#include "glm.hpp"

/* Needed Components */
#include "ECS/Components/Collider.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"

using namespace Mupfel;

void Mupfel::CollisionProcessor::DetectAndResolve(Entity a, Entity b)
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

	/* Pretty ugly for now */

	/* Circle - Circle */
	if (col_a.info.type == ShapeType::Circle && col_b.info.type == ShapeType::Circle)
	{
		CircleCircle(a, b);
	} else
	/* AABB - AABB */
	if (col_a.info.type == ShapeType::AABB && col_b.info.type == ShapeType::AABB)
	{
		AABBAABB(a, b);
	} else
	/* Circle - AABB */
	if (col_a.info.type == ShapeType::AABB && col_b.info.type == ShapeType::Circle ||
		col_a.info.type == ShapeType::Circle && col_b.info.type == ShapeType::AABB)
	{
		CircleAABB(a, b);
	}
	else
	{
		assert(0);
	}

	return;
}

void Mupfel::CollisionProcessor::CircleCircle(Entity a, Entity b)
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
		return;
	}

	/* We have a collision */

	glm::vec2 normal;
	float penetration;

	/* Catch the case of distance_squared being 0, that means the two entities are exactly on top of each other */
	if (distance_squared < 1e-6f)
	{
		normal = glm::vec2(1.0f, 0.0f);
		penetration = radii_added;
	}
	else {
		float distance = sqrt(distance_squared);

		penetration = radii_added - distance;

		normal = glm::vec2(distance_x / distance, distance_y / distance);
	}

	assert(registry.HasComponent<Movement>(a));
	assert(registry.HasComponent<Movement>(b));
	Movement& movement_a = registry.GetComponent<Movement>(a);
	Movement& movement_b = registry.GetComponent<Movement>(b);

	/* Lets add a tiny bit more to make sure that the circles do not collide anymore after this */
	penetration += 0.01f;

	/* Separation */
	glm::vec2 scaled_spearation_vector = normal * (penetration / 2.0f);

	/* Calculate the new positions */
	glm::vec2 new_pos_a = glm::vec2(transform_a.pos_x, transform_a.pos_y) - scaled_spearation_vector;
	glm::vec2 new_pos_b = glm::vec2(transform_b.pos_x, transform_b.pos_y) + scaled_spearation_vector;

	/* Update the components */
	transform_a.pos_x = new_pos_a.x;
	transform_a.pos_y = new_pos_a.y;
	transform_b.pos_x = new_pos_b.x;
	transform_b.pos_y = new_pos_b.y;

	/* Velocity correction */
	glm::vec2 relative_velocity = glm::vec2(movement_b.velocity_x, movement_b.velocity_y) - glm::vec2(movement_a.velocity_x, movement_a.velocity_y);

	float projection = glm::dot(relative_velocity, normal);

	/* We only need to correct the velocity if the two entities are moving towards each other */
	if (projection < 0.0f)
	{
		float e = 1.0f;
		float invMassA = 1.0f;
		float invMassB = 1.0f;

		float impulse_magnitude = -(1.0f + e) * projection / (invMassA + invMassB);
		glm::vec2 impulse_vector = impulse_magnitude * normal;
		movement_a.velocity_x -= impulse_vector.x;
		movement_a.velocity_y -= impulse_vector.y;

		movement_b.velocity_x += impulse_vector.x;
		movement_b.velocity_y += impulse_vector.y;
	}

}

void Mupfel::CollisionProcessor::CircleAABB(Entity a, Entity b)
{
	return;
}

void Mupfel::CollisionProcessor::AABBAABB(Entity a, Entity b)
{
	return;
}
