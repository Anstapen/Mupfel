#pragma once

#include "Core/Layer.h"

class HelloWorldLayer : public Mupfel::Layer
{
	void OnInit() override;
	void OnUpdate(float timestep) override;
	void OnRender() override;
};

