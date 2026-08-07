#pragma once
#include "Core/Scene.h"

class MainMenu : public Mupfel::Scene
{
public:
	MainMenu(Mupfel::SceneHandle in_handle, const std::string& name, Mupfel::Camera cam = {})
		: Mupfel::Scene(in_handle, name, cam)
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
};
