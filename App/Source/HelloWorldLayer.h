#pragma once
#include "Mupfel.h"
#include <random>

class HelloWorldLayer : public Mupfel::Layer
{
public:
	HelloWorldLayer();

	void OnInit() override;
	void OnUpdate(double timestep) override;
	void OnRender() override;
private:
	void ProcessEvents();

private:
	Mupfel::SceneHandle level = Mupfel::Scene::INVALID_HANDLE;
	Mupfel::SceneHandle mainMenu = Mupfel::Scene::INVALID_HANDLE;
	Mupfel::SceneHandle gravityTest = Mupfel::Scene::INVALID_HANDLE;

	std::shared_ptr<Mupfel::ResourceManager> mngr = nullptr;

	Mupfel::Logger::SafeLoggerPtr logger;
};

