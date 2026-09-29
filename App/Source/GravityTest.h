#pragma once
#include "Mupfel.h"

class GravityTest : public Mupfel::Scene
{
public:
	GravityTest(Mupfel::SceneHandle in_handle, const Mupfel::SceneDefinition& def, const std::shared_ptr<Mupfel::ResourceManager> &in_mngr)
		: Mupfel::Scene(in_handle, def, in_mngr)
	{
	}

	void OnInit() final;
	void OnUpdate(double timestep) final;
	void OnRender() final;
};
