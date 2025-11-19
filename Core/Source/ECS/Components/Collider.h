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

	struct SpatialInfo {
		static constexpr uint32_t n_memorised_cells = 16;
		Coordinate<uint32_t> old_cell_min = { 0,0 };
		Coordinate<uint32_t> old_cell_max = { 0,0 };
		std::array<CellIndex, n_memorised_cells> refs = { 0, 0 };
		uint32_t num_cells = 0;
		float bounding_box_size = 16.0f;
	};

	static_assert(sizeof(SpatialInfo) == 152);

	static_assert(sizeof(ColliderInfo) == 32);

	class Collider
	{
	public:
		float GetBoundingBox() const;
		void SetCircle(float in_radius);
	public:
		ColliderInfo info;
	private:
		/* Circle */
		float radius = 0;
		float pad[3] = { 0 };
	private:
		/* General info regarding the spatial indexing */
		SpatialInfo spatial;
	};

	static_assert(sizeof(Collider) == 200);

}
