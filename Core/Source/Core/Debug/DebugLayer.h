#pragma once
#include "Core/Layer.h"
#include <cstdint>

namespace Mupfel {
	class DebugLayer : public Layer
	{
	public:
		void OnInit() override;
		void OnUpdate(double timestep) override;
		void OnRender() override;
	private:
		void DrawDebugInfo();
		void DrawCollisionGrid();
		void DrawEntityColliders();
		void DrawEntityVelocity();
		void DrawEntityIndex();
		void DrawPerformanceMetrics();
		void DrawDebugGUI();
	private:
		static const uint32_t anchor_x = 10;
		static const uint32_t anchor_y = 70;
		bool show_perf = false;
		bool show_collider = false;
		bool show_velocity = false;
		bool show_grid = false;
		bool single_stepping = true;
		bool show_entity_index = false;
	};
}


