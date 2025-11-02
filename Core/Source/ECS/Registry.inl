#pragma once

#include "View.h"
#include "Core/Application.h"
#include <future>
#include <algorithm>
#include "Core/ThreadPool.h"

namespace Mupfel {

	template<typename... Components>
	inline View<Components...> Registry::view() {
		return View<Components...>(*this);
	}


	template<typename ...Components, typename F>
	inline void Registry::ParallelForEach(F&& function, std::vector<Entity>& changed_entities)
	{
		auto view = this->view<Components...>();

		auto& pool = Application::GetCurrentThreadPool();
		const uint32_t num_threads = pool.GetThreadCount();

		using BaseComponent = std::tuple_element_t<0, std::tuple<Components...>>;
		auto& array = GetComponentArray<BaseComponent>();
		const auto& dense = array.dense;
		const uint32_t total = dense.size();

		auto arrays = std::make_tuple(&GetComponentArray<Components>()...);

		if (total == 0)
		{
			return;
		}

		const uint32_t chunk = (total + num_threads - 1) / num_threads;
		std::vector<std::future<std::vector<Entity>>> jobs;
		jobs.reserve(num_threads);


		const auto required = Registry::ComponentSignature<Components...>();

		for (uint32_t t = 0; t < num_threads; t++)
		{
			const uint32_t begin = t * chunk;
			const uint32_t end = std::min(begin + chunk, total);

			if (begin >= end)
			{
				continue;
			}

			jobs.push_back(pool.Enqueue([this, begin, end, function, &dense, required, &arrays]() mutable -> std::vector<Entity> {

				std::vector<Entity> local_entities;
				local_entities.reserve(64);

				for (size_t i = begin; i < end; ++i)
				{
					Entity e{ dense[i] };
					const auto& sig = GetSignature(e.Index());

					/* check if the entity has all the needed components */
					if ((sig & required) != required)
						continue;

					/* Call given function on the entity */
					//bool entity_changed = function(e, GetComponent<Components>(e)...);

					std::apply([&](auto*... arr) {
						bool entity_changed = function(e, arr->Get(e)...);
						if (entity_changed) local_entities.push_back(e);
						}, arrays);

				}

				return local_entities;
				}));

		}

		/* Wait for all jobs to finish and collect the entities */
		for (auto& job : jobs)
		{
			auto local = job.get();
			changed_entities.insert(changed_entities.end(),
				std::make_move_iterator(local.begin()),
				std::make_move_iterator(local.end()));
		}
	}

}