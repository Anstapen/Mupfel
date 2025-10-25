#pragma once
#include "MovementSystem.h"

namespace Mupfel {

	class MovementSystemGPU :
		public MovementSystem
	{
	public:
		MovementSystemGPU(Registry& in_reg);
		void Init() override;
		void DeInit() override;
		void Update(double elapsedTime) override;
	};

}


