#pragma once
#include "ECS/Registry.h"
#include "Core/EventSystem.h"

namespace Mupfel {

	
	class CollisionSystem
	{
		friend class DebugLayer;
	public:
	public:
		CollisionSystem(Registry& reg, EventSystem& evt_sys);
		void Init();
		void Update();
	private:
		void SetCallbacks();
	private:
		Registry& registry;
		EventSystem& evt_system;
	};
}



