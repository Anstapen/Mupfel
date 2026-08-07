#pragma once
#include "Mupfel.h"
#include <random>

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
	Mupfel::SceneHandle mainMenu;
};

