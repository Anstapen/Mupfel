#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <future>
#include <memory>
#include <functional>

namespace Mupfel {

	class ThreadPool
	{
	public:
		explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
		virtual ~ThreadPool();
		size_t GetThreadCount() const { return workers.size(); }

		template<typename F, typename... Args>
		auto Enqueue(F&& function, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;
	private:
		std::vector<std::thread> workers;
		std::queue<std::function<void()>> tasks;

		std::mutex q_mutex;
		std::condition_variable cond;
		bool stop;
	};

	template<typename F, typename... Args>
	inline auto ThreadPool::Enqueue(F&& function, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
	{
		
		using ReturnType = std::invoke_result_t<F, Args...>;

		auto task = std::make_shared<std::packaged_task<ReturnType()>>(
			std::bind(std::forward<F>(function), std::forward<Args>(args)...)
		);

		std::future<ReturnType> res = task->get_future();

		{
			std::unique_lock<std::mutex> lock(q_mutex);
			tasks.emplace([task]() { (*task)(); });
		}

		cond.notify_one();
		return res;
	}

}



