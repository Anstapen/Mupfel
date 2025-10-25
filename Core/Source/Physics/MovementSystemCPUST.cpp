#include "MovementSystemCPUST.h"
#include "Core/Profiler.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Velocity.h"
#include "ECS/View.h"
#include "MovementSystemCPUMT.h"

using namespace Mupfel;

Mupfel::MovementSystemCPUST::MovementSystemCPUST(Registry& in_reg) : MovementSystem(in_reg)
{
}

void MovementSystemCPUST::Init()
{
	garbage.reserve(1000);
}

void MovementSystemCPUST::DeInit()
{
	garbage.clear();
}

void MovementSystemCPUST::Update(double elapsedTime)
{
	ProfilingSample prof("Update - SingleThreaded CPU");

	UpdatePositionsAndColliders();

	CleanUpEntities();

}

void MovementSystemCPUST::UpdatePositionsAndColliders()
{
	ProfilingSample prof("UpdatePositionsAndColliders()");

	float delta_time = Application::GetLastFrameTime();

	int screen_height = Application::GetCurrentRenderHeight();
	int screen_width = Application::GetCurrentRenderWidth();

	garbage.clear();

	ComponentArray<BroadCollider>& broad_collider_array = reg.GetComponentArray<BroadCollider>();
	ComponentArray<Transform>& transform_array = reg.GetComponentArray<Transform>();

	auto view = reg.view<Transform, Velocity>();

	for (auto [entity, transform, velocity] : view)
	{
		if (velocity.x == 0.0f || velocity.y == 0.0f)
		{
			/* This Entity currently has a Velocity of 0 */
			continue;
		}

		transform.pos.x += velocity.x * delta_time;
		transform.pos.y += velocity.y * delta_time;

		/* We updated the positional information of the entity, mark it as dirty */
		transform_array.MarkDirty(entity);


		if (transform.pos.x > screen_width || transform.pos.x < 0.0f || transform.pos.y > screen_height || transform.pos.y < 0.0f)
		{
			garbage.push_back(entity);
			continue;
		}

		/* Update the BroadCollider, if there is one */
		if (broad_collider_array.Has(entity))
		{
			auto& broad_collider = broad_collider_array.Get(entity);
			broad_collider.max.x = transform.pos.x + broad_collider.offset.x;
			broad_collider.max.y = transform.pos.y + broad_collider.offset.y;
			broad_collider.min.x = transform.pos.x - broad_collider.offset.x;
			broad_collider.min.y = transform.pos.y - broad_collider.offset.y;

			/* Clamp the top-left values */
			broad_collider.min.x = broad_collider.min.x < 0.0f ? 0.0f : broad_collider.min.x;
			broad_collider.min.y = broad_collider.min.y < 0.0f ? 0.0f : broad_collider.min.y;
		}

	}
}

void MovementSystemCPUST::CleanUpEntities()
{
	ProfilingSample prof("CleanUpEntities");

	for (auto e : garbage)
	{
		reg.DestroyEntity(e);
	}
}