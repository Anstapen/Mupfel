#pragma once

#include "Core/Layer.h"
#include <mutex>
#include "ECS/Entity.h"
#include <vector>

class HelloWorldLayer : public Mupfel::Layer
{
	void OnInit() override;
	void OnUpdate(float timestep) override;
	void OnRender() override;
private:
	void ProcessEvents();
};

