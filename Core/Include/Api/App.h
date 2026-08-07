/**
 * \file   App.h
 * \brief  Application utilities: Initialization, running and stopping.
 * 
 * This header provides easy-to-use functions to manage Mupfel's main
 * application.
 */
#pragma once
#include "Core/Application.h"
#include "Core/Layer.h"
#include <type_traits>

namespace Mupfel::App
{

/**
 * Initializes the engine subsystems and creates the main window.
 *
 * \param spec Application name and window configuration.
 * \return True if initialization succeeded.
 */
inline bool Init(const ApplicationSpecification& spec) { return Application::Get().Init(spec); }

/** Runs the main loop until the window is closed or Stop() is called. */
inline void Run() { Application::Get().Run(); }

/** Stops the main loop. */
inline void Stop() { Application::Get().Stop(); }

/**
 * Constructs a layer of the requested type, calls its OnInit(), and pushes it onto the layer stack.
 *
 * Layers are updated and rendered in the order they were pushed.
 */
template <typename TLayer>
	requires std::is_base_of_v<Layer, TLayer>
inline void PushLayer()
{
	Application::Get().PushLayer<TLayer>();
}

/** Whether the debug overlay is currently toggled on. */
[[nodiscard]] inline bool DebugMode() { return Application::isDebugModeEnabled(); }

} // namespace Mupfel::App
