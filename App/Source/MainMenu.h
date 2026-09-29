#pragma once
#include "Core/Scene.h"

class MainMenu : public Mupfel::Scene
{
public:
	MainMenu(Mupfel::SceneHandle in_handle, const Mupfel::SceneDefinition &def, std::shared_ptr<Mupfel::ResourceManager> in_mngr)
		: Mupfel::Scene(in_handle, def, std::move(in_mngr))
	{
	}

	void OnInit() final;
	void OnUpdate(double timestep) final;
	void OnRender() final;
};
