#include "MovementSystemCPUMT.h"
#include "Core/Profiler.h"
#include "Core/Application.h"
#include "ECS/Components/Velocity.h"
#include "ECS/Components/Transform.h"
#include "ECS/View.h"

using namespace Mupfel;

MovementSystemCPUMT::MovementSystemCPUMT(Registry& in_reg) : MovementSystem(in_reg)
{

}

void Mupfel::MovementSystemCPUMT::Init()
{
	garbage.reserve(1000);
	dirty.reserve(1000);
}

void Mupfel::MovementSystemCPUMT::DeInit()
{
	garbage.clear();
	dirty.clear();
}

void Mupfel::MovementSystemCPUMT::Update(double elapsedTime)
{
	ProfilingSample prof("Update - CPU Multithreaded");
	UpdatePositionsAndColliders();
	MarkDirtyEntities();
	CleanUpEntities();
}


void MovementSystemCPUMT::UpdatePositionsAndColliders()
{
	ProfilingSample prof("UpdateEntityPositions");

	float delta_time = Application::GetLastFrameTime();


	int screen_height = Application::GetCurrentRenderHeight();
	int screen_width = Application::GetCurrentRenderWidth();

	garbage.clear();
	dirty.clear();

	ComponentArray<BroadCollider>& broad_collider_array = reg.GetComponentArray<BroadCollider>();

	reg.ParallelForEach<Transform, Velocity>([this, delta_time, screen_width, screen_height, &broad_collider_array](Entity e, Transform& t, Velocity& v)
		{

			if (v.x == 0.0f || v.y == 0.0f)
			{
				/* This Entity currently has a Velocity of 0 */
				return false;
			}

			t.pos.x += v.x * delta_time;
			t.pos.y += v.y * delta_time;


			if (t.pos.x > screen_width || t.pos.x < 0.0f || t.pos.y > screen_height || t.pos.y < 0.0f)
			{
				std::scoped_lock lock(garbage_mutex);
				garbage.push_back(e);
			}

			/* Update the BroadCollider, if there is one */
			if (broad_collider_array.Has(e))
			{
				auto& broad_collider = broad_collider_array.Get(e);
				broad_collider.max.x = t.pos.x + broad_collider.offset.x;
				broad_collider.max.y = t.pos.y + broad_collider.offset.y;
				broad_collider.min.x = t.pos.x - broad_collider.offset.x;
				broad_collider.min.y = t.pos.y - broad_collider.offset.y;

				/* Clamp the top-left values */
				broad_collider.min.x = broad_collider.min.x < 0.0f ? 0.0f : broad_collider.min.x;
				broad_collider.min.y = broad_collider.min.y < 0.0f ? 0.0f : broad_collider.min.y;
			}

			return true;

		},
		dirty
	);
}

void MovementSystemCPUMT::CleanUpEntities()
{
	ProfilingSample prof("CleanUpEntities()");

	for (auto e : garbage)
	{
		reg.DestroyEntity(e);
	}
}

void Mupfel::MovementSystemCPUMT::MarkDirtyEntities()
{
	ProfilingSample prof("MarkDirtyEntities()");

	ComponentArray<Transform>& transform_array = reg.GetComponentArray<Transform>();

	for (auto e : dirty)
	{
		transform_array.MarkDirty(e);
	}
}
