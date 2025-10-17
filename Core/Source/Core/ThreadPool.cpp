#include "ThreadPool.h"

Mupfel::ThreadPool::ThreadPool(size_t num_threads) : stop(false)
{
	for (size_t i = 0; i < num_threads; i++)
	{
		/* Every worker function just pops new tasks from the task list */
		workers.emplace_back(
			[this]() {
				while (true) {
					std::function<void()> task;

					{
						std::unique_lock<std::mutex> lock(q_mutex);
						cond.wait(lock, [this]() {
							return stop || !tasks.empty();
							});
						if (stop && tasks.empty())
						{
							return;
						}
						task = std::move(tasks.front());
						tasks.pop();
					}

					task();
				}
			});
	}
}

Mupfel::ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(q_mutex);
		stop = true;
	}
	cond.notify_all();

	for (auto& worker : workers)
	{
		worker.join();
	}
}
