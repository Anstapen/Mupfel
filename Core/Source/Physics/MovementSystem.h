#pragma once
#include "ECS/Registry.h"
#include <cstdint>

namespace Mupfel {
	class MovementSystem
	{
	public:
		MovementSystem(Registry& in_reg, uint32_t in_max_entities = 1000);
		virtual ~MovementSystem() = default;
		virtual void Init() = 0;
		virtual void DeInit() = 0;
		virtual void Update(double elapsedTime) = 0;
	protected:
		Registry& reg;
		uint32_t max_entities;
	};
}


