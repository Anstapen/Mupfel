#pragma once
#include "ECS/Registry.h"
#include "Core/EventSystem.h"
#include "box2d/box2d.h"
#include <cstdint>

namespace Mupfel {

	
	class CollisionSystem
	{
		friend class DebugLayer;
	public:
	public:
		CollisionSystem(Registry& reg, EventSystem& evt_sys);
		void Init();
		void Step(double delta, uint32_t sub_steps);
		void SyncTransforms();
		void DispatchEvents();
		void DeInit();
	private:
		Registry& registry;
		EventSystem& evt_system;
		b2WorldId	 worldId;
	};
}



