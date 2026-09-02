#pragma once
#include "Scene.h"

namespace Mupfel
{
class DefaultScene : public Scene
{
public:
	DefaultScene(SceneHandle in_handle, const SceneDefinition &def) : Scene(in_handle, def) {}

	void OnInit() final;
	void OnUpdate(double timestep) final;
	void OnRender() final;
	void OnSwitchIn() final;
	void OnSwitchOut() final;
};
} // namespace Mupfel
