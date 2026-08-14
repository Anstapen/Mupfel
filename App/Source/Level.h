#pragma once
#include "Mupfel.h"
#include "Player.h"
#include <string>
#include <unordered_map>

class Level : public Mupfel::Scene
{
public:
	Level(Mupfel::SceneHandle in_handle, const std::string& name, Mupfel::Camera cam = {})
		: Mupfel::Scene(in_handle, name, cam), player(Mupfel::Application::GetCurrentRegistry())
	{
	}

	void OnInit() final;
	void OnUpdate(double timestep) final;
	void OnRender() final;

private:
	Player player;
};
