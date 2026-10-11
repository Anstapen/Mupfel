#pragma once
#include "Core/Scene.h"

class MainMenu : public Mupfel::Scene
{
public:
	MainMenu(Mupfel::SceneHandle in_handle, const Mupfel::SceneDefinition &def)
		: Mupfel::Scene(in_handle, def)
	{
	}

	void OnInit() final;
	void OnUpdate(double timestep) final;
	void OnRender() final;
};
