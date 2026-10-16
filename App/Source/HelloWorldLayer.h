#pragma once
#include "Mupfel.h"
#include <unordered_map>
#include "SceneSwitches.h"

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
    std::unordered_map<AppScene, Mupfel::SceneHandle> scenes;

    std::shared_ptr<Mupfel::ResourceManager> mngr = nullptr;

    Mupfel::Logger::SafeLoggerPtr logger;
};
