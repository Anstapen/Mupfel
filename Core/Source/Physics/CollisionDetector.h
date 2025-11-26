#pragma once
#include "ECS/Entity.h"
#include <optional>
#include "glm.hpp"

namespace Mupfel {

	class CollisionDetector
	{
	public:
		struct Result {
			glm::vec2 normal;
			float penetration;
		};

		typedef std::optional<Result> PossibleCollision;
	public:
		static PossibleCollision Colliding(Entity a, Entity b);
	private:
		static PossibleCollision CircleCircle(Entity a, Entity b);
		static PossibleCollision CircleAABB(Entity a, Entity b);
		static PossibleCollision AABBAABB(Entity a, Entity b);
	};

}


