#pragma once
#include "Scene.h"

namespace Mupfel
{
class DefaultScene : public Scene
{
public:
	DefaultScene(SceneHandle in_handle, const std::string& name, Camera cam) : Scene(in_handle, name, cam) {}

	void OnInit() final;
	void OnUpdate(double timestep) final;
	void OnRender() final;
	void OnSwitchIn() final;
	void OnSwitchOut() final;

private:
	void Serialize(const std::string& path) final;
	void Deserialize(const std::string& path) final;
};
} // namespace Mupfel
