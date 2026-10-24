#pragma once
#include "Mupfel.h"
#include "Player.h"
#include "FS/EntityFileManager.h"

class Level : public Mupfel::Scene
{
public:
	Level(Mupfel::SceneHandle in_handle, const Mupfel::SceneDefinition &def)
		: Mupfel::Scene(in_handle, def)
	{
	}

	void OnInit() final;
	void OnUpdate(double timestep) final;
	void OnRender() final;

private:
	void UpdateUserInputs();

private:
	Player player;
	Mupfel::EntityFileManager e_mngr;
};
