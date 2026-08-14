#pragma once
#include "Core/Layer.h"
#include <cstdint>
#include "glm/glm.hpp"

namespace Mupfel {
	class DebugLayer : public Layer
	{
	public:
		void OnInit() override;
		void OnUpdate(double timestep) override;
		void OnRender() override;
	private:
		void DrawPerformanceMetrics();
		void DrawCameraControls();
		void DrawEntityColliders();
		void UpdateMVP();
	private:
		glm::mat4 view;
		glm::mat4 proj;
	};
}


