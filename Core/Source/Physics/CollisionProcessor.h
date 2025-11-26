#pragma once
#include "ECS/Entity.h"

namespace Mupfel {

	class CollisionProcessor
	{
	public:
		static void DetectAndResolve(Entity a, Entity b);
	private:
		static void CircleCircle(Entity a, Entity b);
		static void CircleAABB(Entity a, Entity b);
		static void AABBAABB(Entity a, Entity b);
	};

}


