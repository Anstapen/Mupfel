#pragma once
#include "Mupfel.h"

class GravityTest : public Mupfel::Scene
{
public:
	GravityTest(Mupfel::SceneHandle in_handle, const std::string& name, Mupfel::Camera cam = {})
		: Mupfel::Scene(in_handle, name, cam)
	{
	}

	void OnInit() final;
	void OnUpdate(double timestep) final;
	void OnRender() final;
};
