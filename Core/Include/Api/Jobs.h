/**
 * \file   Jobs.h
 * \brief  Dispatching work onto the engine thread pool.
 *
 */
#pragma once
#include "Core/Application.h"
#include "Core/ThreadPool.h"
#include <concepts>
#include <cstddef>
#include <future>
#include <type_traits>
#include <utility>

namespace Mupfel::Jobs
{

/**
 * Queues function(args...) for execution on a worker thread.
 *
 * \return A future for the result.
 * \throws std::runtime_error if the pool has already shut down.
 */
template <typename F, typename... Args>
	requires std::invocable<F, Args...>
[[nodiscard]] inline auto Enqueue(F&& function, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
{
	return Application::GetCurrentThreadPool().Enqueue(std::forward<F>(function), std::forward<Args>(args)...);
}

} // namespace Mupfel::Jobs
