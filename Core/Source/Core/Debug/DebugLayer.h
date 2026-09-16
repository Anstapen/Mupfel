#pragma once
#include "Core/Layer.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Collider.h"
#include "ECS/Components/Sensor.h"
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
		void DrawCircleCollider(Transform& t, Collider& c);
		void DrawBoxCollider(Transform& t, Collider& c);
		void DrawCapsuleCollider(Transform& t, Collider& c);
		void DrawCircleSensor(Transform& t, Sensor& s);
		void DrawBoxSensor(Transform& t, Sensor& s);
		void DrawCapsuleSensor(Transform& t, Sensor& s);
	};
}


