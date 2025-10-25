#pragma once
#include "ECS/Registry.h"

namespace Mupfel {
	class MovementSystem
	{
	public:
		MovementSystem(Registry& in_reg);
		virtual ~MovementSystem() = default;
		virtual void Init() = 0;
		virtual void DeInit() = 0;
		virtual void Update(double elapsedTime) = 0;
	protected:
		Registry& reg;
	};
}


