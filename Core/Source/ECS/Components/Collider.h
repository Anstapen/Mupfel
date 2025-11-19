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

	struct alignas(16) SpatialInfo {
		static constexpr uint32_t n_memorised_cells = 16;
		Coordinate<uint32_t> old_cell_min = { 0,0 };
		Coordinate<uint32_t> old_cell_max = { 0,0 };
		std::array<CellIndex, n_memorised_cells> refs = { 0, 0 };
		uint32_t num_cells = 0;
		float bounding_box_size = 16.0f;
	};

	static_assert(sizeof(SpatialInfo) == 160);

	static_assert(sizeof(ColliderInfo) == 32);

#if 0
	class alignas(16) Collider
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
#else
	struct Collider
	{
		float GetBoundingBox() const;
		void SetCircle(float in_radius);
		// --- ColliderInfo / Meta ---
		uint32_t type;         // ShapeType als uint (z.B. 0=None, 1=Circle, ...)
		uint32_t layer;        // Bitmaske für Layer
		uint32_t mask;         // Mit welchen Layern kollidiere ich
		uint32_t flags;        // is_trigger, send_events, etc.

		uint32_t callback_id;  // ID in deiner Callback-Registry
		uint32_t padA;         // Padding, reserviert für spätere Nutzung
		uint32_t padB;         // "
		uint32_t padC;         // "

		// --- Shape-Daten (Circle für jetzt) ---
		float radius;          // Circle-Radius
		float padShape1;       // Platzhalter für zukünftige Shape-Daten
		float padShape2;       // "
		float padShape3;       // "

		// --- Spatial/Broadphase-Daten ---
		// alte Zellen (min und max) im Grid (world-space in Zellen koordiniert)
		uint32_t old_cell_min_x;
		uint32_t old_cell_min_y;
		uint32_t old_cell_max_x;
		uint32_t old_cell_max_y;

		// gemerkte Zellen (z.B. für Multicell-Overlap)
		static constexpr uint32_t n_memorised_cells = 16;
		CellIndex cell_indices[n_memorised_cells];

		uint32_t num_cells;        // wie viele Einträge in cell_indices gültig sind
		float    bounding_box_size;// Kantenlänge der Bounding-AABB (z.B. für Grid-Eintragung)
		uint32_t padSpatial1;      // Padding / Reserve
		uint32_t padSpatial2;      // Padding / Reserve
	};

	static_assert(sizeof(Collider) == 208, "ColliderGPU size must match GLSL struct");
	static_assert(alignof(Collider) % 4 == 0, "ColliderGPU must be at least 4-byte aligned");
#endif

}
