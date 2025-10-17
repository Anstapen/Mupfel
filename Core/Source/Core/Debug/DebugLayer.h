#pragma once
#include "Core/Layer.h"

namespace Mupfel {
	class DebugLayer : public Layer
	{
	public:
		void OnInit() override;
		void OnUpdate(float timestep) override;
		void OnRender() override;
	};
}


