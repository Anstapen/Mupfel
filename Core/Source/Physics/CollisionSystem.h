#pragma once
#include "ECS/Components/SpatialInfo.h"
#include <array>
#include <vector>
#include <future>
#include "Core/Coordinate.h"
#include "Core/Debug/DebugLayer.h"
#include "ECS/Entity.h"
#include "ECS/Components/BroadCollider.h"
#include "ECS/ComponentArray.h"
#include "GPUCollisionGrid.h"

namespace Mupfel {

	class Registry;
	class EventSystem;
	
	class CollisionSystem
	{
		friend class DebugLayer;
	public:
		CollisionSystem(Registry& reg, EventSystem& evt_sys);
		void Init();
		void Update();
	private:
		uint32_t WorldtoCell(Coordinate<uint32_t> c);
		uint32_t PointXtoCell(uint32_t x);
		uint32_t PointYtoCell(uint32_t y);


		void SetCallbacks();
		void Join();
		void SetProgramParams();
		void UpdateCells();
		void CheckInteractions();
		void CheckEntityInteraction(uint32_t start, uint32_t num_ents);
		void ClearGrid();
	private:
		Registry& registry;
		EventSystem& evt_system;
		GPUCollisionGrid collision_grid;
	};
}



