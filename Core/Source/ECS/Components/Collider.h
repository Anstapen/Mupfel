#pragma once
#include "Core/Coordinate.h"
#include <cstdint>
#include <array>
#include <bitset>
#include "Physics/ShapeType.h"

namespace Mupfel {

	struct CellIndex {
		uint32_t cell_id;
		uint32_t entity_id;
	};

	struct ColliderInfo {
		ShapeType type = ShapeType::None;
		uint32_t layer = 0;
		uint32_t mask = 0;
		uint32_t flags = 0;
		uint32_t callback_id = 0;
	private:
		float pad[3] = { 0 };
	};


	static_assert(sizeof(ColliderInfo) == 32);

	class Collider
	{
	public:
		float GetBoundingBoxX() const;
		float GetBoundingBoxY() const;
		void SetCircle(float in_radius);
		float GetCircle() const;
		void SetAABB(float x, float y);

	public:
		ColliderInfo info;
	private:
		/* Circle */
		float radius = 0;
		float pad[3] = { 0 };
	private:
		float bounding_box_x = 16.0f;
		float bounding_box_y = 16.0f;
	};

	static_assert(sizeof(Collider) == 56);

}
