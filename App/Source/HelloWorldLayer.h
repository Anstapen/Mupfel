#pragma once

#include "Core/Layer.h"
#include "Renderer/ImageManager.h"
#include <random>
#include "ECS/Entity.h"
#include "Core/Scene.h"

class HelloWorldLayer : public Mupfel::Layer
{
public:
	HelloWorldLayer();

private:
	void OnInit() override;
	void OnUpdate(double timestep) override;
	void OnRender() override;
private:
	void ProcessEvents();

private:
	Mupfel::SceneHandle level;
};

