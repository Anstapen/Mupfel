/**
 * \file   Screen.h
 * \brief  The render surface: size and minimized state.
 *
 */
#pragma once
#include "Core/Application.h"

namespace Mupfel::Screen
{

/** Current width of the render surface, in pixels. */
[[nodiscard]] inline int Width() { return Application::GetCurrentRenderWidth(); }

/** Current height of the render surface, in pixels. */
[[nodiscard]] inline int Height() { return Application::GetCurrentRenderHeight(); }

/** Whether the window is currently minimized. */
[[nodiscard]] inline bool Minimized() { return Application::IsWindowMinimized(); }

} // namespace Mupfel::Screen
