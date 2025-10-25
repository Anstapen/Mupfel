#pragma once
#include "MovementSystem.h"
#include "ECS/Entity.h"
#include <memory>

namespace Mupfel {

	class MovementSystemCPUST :
		public MovementSystem
	{
	public:
		MovementSystemCPUST(Registry& in_reg);
		void Init() override;
		void DeInit() override;
		void Update(double elapsedTime) override;

	private:
		void UpdatePositionsAndColliders();
		void CleanUpEntities();

	private:
		std::vector<Entity> garbage;
	};

}

