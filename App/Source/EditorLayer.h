#pragma once
#include "Core/Layer.h"
#include "Physics/ShapeType.h"

class EditorLayer : public Mupfel::Layer
{
private:
	void OnInit() override;
	void OnUpdate(double timestep) override;
	void OnRender() override;
};

