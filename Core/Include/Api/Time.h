/**
 * \file   Time.h
 * \brief  Frame timing.
 *
 */
#pragma once
#include "Core/Application.h"
#include <cstdint>

namespace Mupfel::Time
{

/** Seconds elapsed since engine startup. */
[[nodiscard]] inline double Now() { return Application::GetTime(); }

/** Duration of the most recently completed frame, in seconds. */
[[nodiscard]] inline float Delta() { return Application::GetLastFrameTime(); }

/** Number of frames run since startup. */
[[nodiscard]] inline uint64_t Frame() { return Application::GetFrameCount(); }

} // namespace Mupfel::Time
