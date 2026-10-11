#pragma once
#include "Mupfel.h"

class GravityTest : public Mupfel::Scene
{
public:
	GravityTest(Mupfel::SceneHandle in_handle, const Mupfel::SceneDefinition& def)
		: Mupfel::Scene(in_handle, def)
	{
	}

	void OnInit() final;
	void OnUpdate(double timestep) final;
	void OnRender() final;
};
