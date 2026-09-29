#pragma once
#include "Mupfel.h"
#include "Player.h"
#include <memory>
#include "GameObjects/Interactable.h"

class Level : public Mupfel::Scene
{
public:
	Level(Mupfel::SceneHandle in_handle, const Mupfel::SceneDefinition &def, const std::shared_ptr<Mupfel::ResourceManager>& in_mngr)
		: Mupfel::Scene(in_handle, def, in_mngr), player(Mupfel::Application::GetCurrentRegistry(), in_mngr)
	{
	}

	void OnInit() final;
	void OnUpdate(double timestep) final;
	void OnRender() final;

private:
	void UpdateUserInputs();

private:
	Player player;
	std::vector<std::unique_ptr<Interactable>> interactables;
};
