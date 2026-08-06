#include "Scene.h"

using namespace Mupfel;

SceneHandle Mupfel::Scene::GetHandle() const { return handle; }

const Camera& Mupfel::Scene::GetCamera() const
{ return camera; }
