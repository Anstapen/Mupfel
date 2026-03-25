#include "CollisionProcessor.h"
#include <cassert>
#include "Core/Application.h"
#include <cmath>
#include <array>

#include "glm.hpp"

/* Needed Components */
#include "ECS/Components/Collider.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"

using namespace Mupfel;

typedef std::array<std::array< CollisionProcessor::CollisionResolver, (size_t)ShapeType::COUNT>, (size_t)ShapeType::COUNT> CollisionFunctionTable;

static CollisionFunctionTable collision_function_table = { nullptr };

#include <iostream>
void Mupfel::CollisionProcessor::DetectAndResolve(Entity a, Entity b)
{
	std::cout << "Frame: " << Application::GetFrameCount() << ", Collision detected between Entity " << a.Index() << " and Entity " << b.Index() << std::endl;
	Registry& registry = Application::GetCurrentRegistry();

	assert(registry.HasComponent<Collider>(a));
	assert(registry.HasComponent<Collider>(b));

	Collider col_a = registry.GetComponent<Collider>(a);
	Collider col_b = registry.GetComponent<Collider>(b);

	size_t index_a = static_cast<size_t>(col_a.info.type);
	size_t index_b = static_cast<size_t>(col_b.info.type);

	CollisionResolver resolver = collision_function_table[index_a][index_b];
	if (resolver != nullptr)
	{
		resolver(a, b);
	}

	return;
}

bool Mupfel::CollisionProcessor::RegisterCollisionResolver(ShapeType type_a, ShapeType type_b, CollisionResolver resolver)
{
	size_t index_a = static_cast<size_t>(type_a);
	size_t index_b = static_cast<size_t>(type_b);

	/* We do not allow duplicate registration */
	if(collision_function_table[index_a][index_b] != nullptr)
	{
		return false;
	}

	collision_function_table[index_a][index_b] = resolver;
	collision_function_table[index_b][index_a] = [resolver](Entity a, Entity b)
		{
			resolver(b, a);
		};;

	return true;
}

void Mupfel::CollisionProcessor::CircleCircle(Entity a, Entity b)
{
	Registry& registry = Application::GetCurrentRegistry();

	assert(registry.HasComponent<Transform>(a));
	assert(registry.HasComponent<Transform>(b));
	assert(registry.HasComponent<Collider>(a));
	assert(registry.HasComponent<Collider>(b));

	Collider& collider_a = registry.GetComponent<Collider>(a);
	Collider& collider_b = registry.GetComponent<Collider>(b);

	Transform& transform_a = registry.GetComponent<Transform>(a);
	Transform& transform_b = registry.GetComponent<Transform>(b);

	assert(collider_a.info.type == ShapeType::Circle);
	assert(collider_b.info.type == ShapeType::Circle);

	float dx = transform_b.pos_x - transform_a.pos_x;
	float dy = transform_b.pos_y - transform_a.pos_y;

	float dist_sq = dx * dx + dy * dy;

	float r = collider_a.GetCircle() + collider_b.GetCircle();
	float r_sq = r * r;

	if (dist_sq >= r_sq)
		return;

	// --- Collision ---
	glm::vec2 normal;
	float penetration;

	if (dist_sq < 1e-6f)
	{
		normal = glm::vec2(1.0f, 0.0f);
		penetration = r;
	}
	else
	{
		float dist = sqrt(dist_sq);
		normal = glm::vec2(dx / dist, dy / dist);
		penetration = r - dist;
	}

	penetration += 0.01f;

	// --- Movement availability ---
	bool hasMoveA = registry.HasComponent<Movement>(a);
	bool hasMoveB = registry.HasComponent<Movement>(b);

	float invMassA = hasMoveA ? 1.0f : 0.0f;
	float invMassB = hasMoveB ? 1.0f : 0.0f;

	float invMassSum = invMassA + invMassB;

	if (invMassSum == 0.0f)
		return; // beide statisch

	// --- Separation ---
	glm::vec2 sep = normal * (penetration / invMassSum);

	transform_a.pos_x -= sep.x * invMassA;
	transform_a.pos_y -= sep.y * invMassA;

	transform_b.pos_x += sep.x * invMassB;
	transform_b.pos_y += sep.y * invMassB;

	// --- Velocity ---
	if (!hasMoveA && !hasMoveB)
		return;

	glm::vec2 velA(0.0f);
	glm::vec2 velB(0.0f);

	if (hasMoveA)
	{
		auto& m = registry.GetComponent<Movement>(a);
		velA = { m.velocity_x, m.velocity_y };
	}

	if (hasMoveB)
	{
		auto& m = registry.GetComponent<Movement>(b);
		velB = { m.velocity_x, m.velocity_y };
	}

	glm::vec2 rel_vel = velB - velA;

	float proj = glm::dot(rel_vel, normal);

	if (proj < 0.0f)
	{
		float e = 1.0f;

		float j = -(1.0f + e) * proj / invMassSum;
		glm::vec2 impulse = j * normal;

		if (hasMoveA)
		{
			auto& m = registry.GetComponent<Movement>(a);
			m.velocity_x -= impulse.x * invMassA;
			m.velocity_y -= impulse.y * invMassA;
		}

		if (hasMoveB)
		{
			auto& m = registry.GetComponent<Movement>(b);
			m.velocity_x += impulse.x * invMassB;
			m.velocity_y += impulse.y * invMassB;
		}
	}
}

void Mupfel::CollisionProcessor::CircleAABB(Entity a, Entity b)
{
	Registry& registry = Application::GetCurrentRegistry();

	Collider& circle = registry.GetComponent<Collider>(a);
	Collider& aabb = registry.GetComponent<Collider>(b);

	Transform& t_circle = registry.GetComponent<Transform>(a);
	Transform& t_aabb = registry.GetComponent<Transform>(b);

	float radius = circle.GetCircle();

	// AABB bounds
	float half_x = aabb.GetBoundingBoxX() * 0.5f;
	float half_y = aabb.GetBoundingBoxY() * 0.5f;

	float min_x = t_aabb.pos_x - half_x;
	float max_x = t_aabb.pos_x + half_x;
	float min_y = t_aabb.pos_y - half_y;
	float max_y = t_aabb.pos_y + half_y;

	// Closest point
	float closest_x = std::clamp(t_circle.pos_x, min_x, max_x);
	float closest_y = std::clamp(t_circle.pos_y, min_y, max_y);

	float dx = t_circle.pos_x - closest_x;
	float dy = t_circle.pos_y - closest_y;

	float dist_sq = dx * dx + dy * dy;

	if (dist_sq >= radius * radius)
		return;

	// --- Collision ---
	glm::vec2 normal;
	float penetration;

	if (dist_sq < 1e-6f)
	{
		float dist_left = std::abs(t_circle.pos_x - min_x);
		float dist_right = std::abs(max_x - t_circle.pos_x);
		float dist_down = std::abs(t_circle.pos_y - min_y);
		float dist_up = std::abs(max_y - t_circle.pos_y);

		float min_dist = std::min({ dist_left, dist_right, dist_down, dist_up });

		if (min_dist == dist_left)      normal = { -1, 0 };
		else if (min_dist == dist_right)normal = { 1, 0 };
		else if (min_dist == dist_down) normal = { 0, -1 };
		else                            normal = { 0, 1 };

		penetration = radius;
	}
	else
	{
		float dist = sqrt(dist_sq);
		normal = glm::vec2(dx / dist, dy / dist);
		penetration = radius - dist;
	}

	penetration += 0.01f;

	// --- Movement availability ---
	bool hasMoveA = registry.HasComponent<Movement>(a);
	bool hasMoveB = registry.HasComponent<Movement>(b);

	float invMassA = hasMoveA ? 1.0f : 0.0f;
	float invMassB = hasMoveB ? 1.0f : 0.0f;

	float invMassSum = invMassA + invMassB;

	if (invMassSum == 0.0f)
		return; // beide statisch

	// --- Separation ---
	glm::vec2 sep = normal * (penetration / invMassSum);

	t_circle.pos_x += sep.x * invMassA;
	t_circle.pos_y += sep.y * invMassA;

	t_aabb.pos_x -= sep.x * invMassB;
	t_aabb.pos_y -= sep.y * invMassB;

	// --- Velocity ---
	if (!hasMoveA && !hasMoveB)
		return;

	glm::vec2 velA(0.0f);
	glm::vec2 velB(0.0f);

	if (hasMoveA)
	{
		auto& m = registry.GetComponent<Movement>(a);
		velA = { m.velocity_x, m.velocity_y };
	}

	if (hasMoveB)
	{
		auto& m = registry.GetComponent<Movement>(b);
		velB = { m.velocity_x, m.velocity_y };
	}

	glm::vec2 rel_vel = velA - velB;

	float proj = glm::dot(rel_vel, normal);

	if (proj < 0.0f)
	{
		float e = 1.0f;

		float j = -(1.0f + e) * proj / invMassSum;
		glm::vec2 impulse = j * normal;

		if (hasMoveA)
		{
			auto& m = registry.GetComponent<Movement>(a);
			m.velocity_x += impulse.x * invMassA;
			m.velocity_y += impulse.y * invMassA;
		}

		if (hasMoveB)
		{
			auto& m = registry.GetComponent<Movement>(b);
			m.velocity_x -= impulse.x * invMassB;
			m.velocity_y -= impulse.y * invMassB;
		}
	}
}

void Mupfel::CollisionProcessor::AABBAABB(Entity a, Entity b)
{
	Registry& registry = Application::GetCurrentRegistry();

	Collider& col_a = registry.GetComponent<Collider>(a);
	Collider& col_b = registry.GetComponent<Collider>(b);

	Transform& t_a = registry.GetComponent<Transform>(a);
	Transform& t_b = registry.GetComponent<Transform>(b);

	float halfA_x = col_a.GetBoundingBoxX() * 0.5f;
	float halfA_y = col_a.GetBoundingBoxY() * 0.5f;

	float halfB_x = col_b.GetBoundingBoxX() * 0.5f;
	float halfB_y = col_b.GetBoundingBoxY() * 0.5f;

	float minAx = t_a.pos_x - halfA_x;
	float maxAx = t_a.pos_x + halfA_x;
	float minAy = t_a.pos_y - halfA_y;
	float maxAy = t_a.pos_y + halfA_y;

	float minBx = t_b.pos_x - halfB_x;
	float maxBx = t_b.pos_x + halfB_x;
	float minBy = t_b.pos_y - halfB_y;
	float maxBy = t_b.pos_y + halfB_y;

	float overlapX = std::min(maxAx, maxBx) - std::max(minAx, minBx);
	float overlapY = std::min(maxAy, maxBy) - std::max(minAy, minBy);

	if (overlapX <= 0.0f || overlapY <= 0.0f)
		return;

	glm::vec2 normal;
	float penetration;

	if (overlapX < overlapY)
	{
		penetration = overlapX;
		normal = (t_a.pos_x < t_b.pos_x) ? glm::vec2(-1, 0) : glm::vec2(1, 0);
	}
	else
	{
		penetration = overlapY;
		normal = (t_a.pos_y < t_b.pos_y) ? glm::vec2(0, -1) : glm::vec2(0, 1);
	}

	penetration += 0.01f;

	// --- Movement availability ---
	bool hasMoveA = registry.HasComponent<Movement>(a);
	bool hasMoveB = registry.HasComponent<Movement>(b);

	float invMassA = hasMoveA ? 1.0f : 0.0f;
	float invMassB = hasMoveB ? 1.0f : 0.0f;

	float invMassSum = invMassA + invMassB;

	if (invMassSum == 0.0f)
		return; // beide statisch

	// --- Separation ---
	glm::vec2 sep = normal * (penetration / invMassSum);

	t_a.pos_x -= sep.x * invMassA;
	t_a.pos_y -= sep.y * invMassA;

	t_b.pos_x += sep.x * invMassB;
	t_b.pos_y += sep.y * invMassB;

	// --- Velocity ---
	if (!hasMoveA && !hasMoveB)
		return;

	glm::vec2 velA(0.0f);
	glm::vec2 velB(0.0f);

	if (hasMoveA)
	{
		auto& m = registry.GetComponent<Movement>(a);
		velA = { m.velocity_x, m.velocity_y };
	}

	if (hasMoveB)
	{
		auto& m = registry.GetComponent<Movement>(b);
		velB = { m.velocity_x, m.velocity_y };
	}

	glm::vec2 rel_vel = velB - velA;

	float proj = glm::dot(rel_vel, normal);

	if (proj < 0.0f)
	{
		float e = 1.0f;

		float j = -(1.0f + e) * proj / invMassSum;
		glm::vec2 impulse = j * normal;

		if (hasMoveA)
		{
			auto& m = registry.GetComponent<Movement>(a);
			m.velocity_x -= impulse.x * invMassA;
			m.velocity_y -= impulse.y * invMassA;
		}

		if (hasMoveB)
		{
			auto& m = registry.GetComponent<Movement>(b);
			m.velocity_x += impulse.x * invMassB;
			m.velocity_y += impulse.y * invMassB;
		}
	}
}
