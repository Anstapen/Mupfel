#pragma once
#include "Mupfel.h"
#include "Player.h"
#include <unordered_map>
#include <string>



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
	void OnSwitchIn() final;
	void OnSwitchOut() final;

private:
	void Serialize(const std::string& path) final;
	void Deserialize(const std::string& path) final;

private:
	std::unordered_map<std::string, Mupfel::ImageHandle> image_map;
	Player												 player;
};
