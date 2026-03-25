#pragma once
#include "ECS/Entity.h"
#include "ShapeType.h"
#include <functional>

namespace Mupfel {

	class CollisionProcessor
	{
	public:
		using CollisionResolver = std::function<void(Entity, Entity)>;
	public:
		static void DetectAndResolve(Entity a, Entity b);
		static bool RegisterCollisionResolver(ShapeType type_a, ShapeType type_b, CollisionResolver resolver);
		static void CircleCircle(Entity a, Entity b);
		static void CircleAABB(Entity a, Entity b);
		static void AABBAABB(Entity a, Entity b);
	};

}


