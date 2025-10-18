#pragma once

#include "Core/Layer.h"
#include <mutex>

class HelloWorldLayer : public Mupfel::Layer
{
	void OnInit() override;
	void OnUpdate(float timestep) override;
	void OnRender() override;
private:
	void DrawDebugInfo();
private:
	std::mutex garbage_mutex;
};

