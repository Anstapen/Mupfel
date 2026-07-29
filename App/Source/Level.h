#pragma once
#include "Core/Scene.h"
#include "Core/Event.h"



class Level : public Mupfel::Scene
{
public:
	Level(Mupfel::SceneHandle in_handle, const std::string& name) : Mupfel::Scene(in_handle, name) {}

	void OnInit() final;
	void OnUpdate() final;
	void OnRender() final;
	void OnSwitchIn() final;
	void OnSwitchOut() final;

private:
	void Serialize(const std::string& path) final;
	void Deserialize(const std::string& path) final;
};
