/**
 * \file   Mupfel.h
 * \brief  The public Mupfel API.
 *
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
#include "Core/CoreEvents.h"
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
#include "ECS/Components/Texture.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Sensor.h"
#include "ECS/Components/Body.h"
#include "ECS/Components/Timer.h"

/* Rendering and UI */
#include "Core/UI.h"
#include "Renderer/Camera.h"
#include "Renderer/Image.h"


/* Utilities */
#include "Core/Error.h"
#include "Core/Profiler.h"

/* Resource Management */
#include "Core/ResourceManager.h"
