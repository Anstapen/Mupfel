/**
 * \file   Scenes.h
 * \brief  Creating, switching and querying scenes.
 *
 */
#pragma once
#include "Core/Application.h"
#include "Core/Scene.h"
#include "Renderer/Camera.h"
#include <string>

namespace Mupfel::Scenes
{

/**
 * Creates a scene of type T.
 * 
 * If \a path is provided, the Deserialize() callback of the given scene is called.
 * After that, OnInit() is called.
 *
 * \param name   Display name; also names the scene's logger.
 * \param path   Optional file to deserialize the scene from.
 * \param camera The scene's starting camera.
 * \return The new scene's handle, or Scene::INVALID_HANDLE if Scene::MAX_SCENES is exhausted.
 */
template <typename T>
	requires SceneType<T>
[[nodiscard]] inline SceneHandle Create(const std::string& name, const std::string& path = {}, Camera camera = {})
{
	return Application::CreateScene<T>(name, path, camera);
}

/**
 * Queue a switch to \a handle.
 *
 */
inline void Switch(SceneHandle handle) { Application::QueueSceneSwitch(handle); }

/** The scene currently being updated and rendered. */
[[nodiscard]] inline SceneHandle Current() { return Application::GetCurrentSceneHandle(); }

/**
 * The active scene's camera, mutable.
 */
[[nodiscard]] inline Camera& CurrentCamera() { return Application::GetCurrentSceneCamera(); }

} // namespace Mupfel::Scenes
