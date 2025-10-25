#pragma once
#include <string_view>
#include <vector>
#include <mutex>
#include <cstdint>

namespace Mupfel {

	struct ProfilingSample {
		ProfilingSample(std::string_view in_name);
		virtual ~ProfilingSample();

		ProfilingSample(ProfilingSample&& other) noexcept;
		ProfilingSample& operator=(ProfilingSample&& other) noexcept;

		ProfilingSample(const ProfilingSample&) noexcept;
		ProfilingSample& operator=(const ProfilingSample&) noexcept;

		/* Only allow stack-allocated objects for RAII to be correct*/
		void* operator new(size_t) = delete;
		void* operator new[](size_t) = delete;

		std::string_view name;
		uint32_t id;
		double start_time;
		double end_time;
		bool active;
		uint32_t depth = 0;;
	};

	class Profiler
	{
		friend struct ProfilingSample;
	public:
		static Profiler& Get();
		static void Clear();
		static bool IsClearing();
		static const std::vector<ProfilingSample>& GetCurrentSamples();
		static uint32_t GetId();
	private:
		void AddSample(ProfilingSample&& sample);
		Profiler() = default;
		~Profiler() = default;
		Profiler(const Profiler&) = delete;
		Profiler& operator=(const Profiler&) = delete;
		Profiler(Profiler&&) = delete;
		Profiler& operator=(Profiler&&) = delete;
	private:
		bool is_clearing = false;
		std::mutex mutex;
		std::vector<ProfilingSample> samples;
	};

}



