#pragma once
#include "Core/Layer.h"

class EditorLayer : public Mupfel::Layer
{
private:
	void OnInit() override;
	void OnUpdate(double timestep) override;
	void OnRender() override;
	void ProcessEvents();
	void CreateNewEntity();
};

