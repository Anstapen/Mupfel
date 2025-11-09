#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <future>
#include <memory>
#include <functional>

namespace Mupfel {

	/**
	 * @brief A simple, thread-safe thread pool implementation.
	 *
	 * The ThreadPool class manages a fixed number of worker threads that continuously
	 * process tasks from a shared queue. Tasks can be enqueued from any thread using
	 * the Enqueue() function, which returns a std::future representing the result of
	 * the asynchronous computation.
	 *
	 * The pool automatically starts the requested number of worker threads upon
	 * construction and gracefully shuts down when destroyed.
	 *
	 * Example usage:
	 * @code
	 * Mupfel::ThreadPool pool(4);
	 * auto future = pool.Enqueue([](int a, int b) { return a + b; }, 3, 7);
	 * int result = future.get(); // returns 10
	 * @endcode
	 */
	class ThreadPool
	{
	public:
		/**
		 * @brief Constructs a thread pool and launches the specified number of worker threads.
		 * @param num_threads Number of threads to create. Defaults to the hardware concurrency.
		 *
		 * Each thread continuously waits for available tasks in the task queue.
		 */
		explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());

		/**
		 * @brief Destructor. Gracefully shuts down the pool.
		 *
		 * Signals all worker threads to stop processing tasks, waits for all
		 * threads to finish execution, and joins them before destruction.
		 */
		virtual ~ThreadPool();

		/**
		 * @brief Returns the number of worker threads currently in the pool.
		 * @return The total number of threads.
		 */
		size_t GetThreadCount() const { return workers.size(); }

		/**
		 * @brief Enqueues a new task for asynchronous execution.
		 *
		 * This function accepts a callable and its arguments, wraps it into a
		 * std::packaged_task, and pushes it into the internal task queue. The function
		 * returns a std::future that can be used to retrieve the result of the task
		 * once it completes.
		 *
		 * The task will be executed by one of the worker threads as soon as it becomes available.
		 *
		 * @tparam F The callable type (function, lambda, functor, etc.)
		 * @tparam Args The argument types passed to the callable.
		 * @param function The callable object to execute.
		 * @param args The arguments to pass to the callable.
		 * @return A std::future containing the result of the callable execution.
		 *
		 * @note This function is thread-safe and may be called from multiple threads concurrently.
		 */
		template<typename F, typename... Args>
		auto Enqueue(F&& function, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

	private:
		/** @brief Vector holding all worker threads. */
		std::vector<std::thread> workers;

		/** @brief Queue storing all pending tasks to be executed by worker threads. */
		std::queue<std::function<void()>> tasks;

		/** @brief Mutex protecting access to the task queue. */
		std::mutex q_mutex;

		/** @brief Condition variable used to notify worker threads of new tasks. */
		std::condition_variable cond;

		/** @brief Flag indicating whether the pool is stopping. */
		bool stop;
	};

	/**
	 * @brief Template implementation of the Enqueue function.
	 *
	 * Wraps the given callable into a packaged_task, pushes it to the task queue,
	 * and returns a std::future representing the result.
	 */
	template<typename F, typename... Args>
	inline auto ThreadPool::Enqueue(F&& function, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
	{
		using ReturnType = std::invoke_result_t<F, Args...>;

		// Create a shared packaged task from the provided function and arguments
		auto task = std::make_shared<std::packaged_task<ReturnType()>>(
			std::bind(std::forward<F>(function), std::forward<Args>(args)...)
		);

		std::future<ReturnType> res = task->get_future();

		{
			std::unique_lock<std::mutex> lock(q_mutex);
			tasks.emplace([task]() { (*task)(); });
		}

		// Notify one waiting worker thread that a new task is available
		cond.notify_one();
		return res;
	}

}



