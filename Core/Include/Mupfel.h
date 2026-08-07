/**
 * \file   Mupfel.h
 * \brief  The public entry point to the Mupfel engine.
 *
 * Including this header gives an application everything it needs to define layers and scenes,
 * drive the ECS, and draw immediate-mode UI. It is the only header a game is expected to
 * include; the individual headers below remain includable on their own if a translation unit
 * wants a narrower (and cheaper to compile) surface.
 *
 * Anything not reachable from this file is engine-internal and lives under `Core/Source`,
 * which is deliberately absent from the application's include path — see `Core/Build-Core.lua`.
 */
#pragma once

/* Easy-to-use functions */
#include "Api/Api.h"

/* Application lifecycle */
#include "Core/Application.h"
#include "Core/Layer.h"
#include "Core/Scene.h"

/* Events and input */
#include "Core/Event.h"
#include "Core/EventSystem.h"
#include "Core/Input.h"
#include "Core/InputManager.h"

/* ECS */
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/View.h"

/* Components */
#include "ECS/Components/Animation.h"
#include "ECS/Components/Collider.h"
#include "ECS/Components/Light.h"
#include "ECS/Components/Movement.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Transform.h"

/* Rendering and UI */
#include "Core/UI.h"
#include "Renderer/Camera.h"
#include "Renderer/ImageManager.h"


/* Utilities */
#include "Core/Error.h"
#include "Core/Profiler.h"
