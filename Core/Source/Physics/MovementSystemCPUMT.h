#pragma once
#include "MovementSystem.h"
#include "ECS/Registry.h"
#include <mutex>

namespace Mupfel {

	class MovementSystemCPUMT :
		public MovementSystem
	{
	public:
		MovementSystemCPUMT(Registry& in_reg);
		void Init() override;
		void DeInit() override;
		void Update(double elapsedTime) override;

	private:
		void UpdatePositionsAndColliders();
		void CleanUpEntities();
		void MarkDirtyEntities();

	private:
		std::mutex garbage_mutex;
		std::vector<Entity> garbage;
		std::vector<Entity> dirty;
	};

}


