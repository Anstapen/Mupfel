#pragma once
#include <string_view>
#include <vector>
#include <mutex>
#include <cstdint>

namespace Mupfel {

	/**
	 * @brief Represents a single profiling sample within the engine.
	 *
	 * The ProfilingSample class provides a lightweight RAII-based mechanism
	 * for measuring execution time between construction and destruction.
	 *
	 * Each sample records its name, start and end timestamps, a depth value
	 * (for nested profiling scopes), and a unique ID assigned by the Profiler.
	 *
	 * ProfilingSample objects are intended to be created on the stack:
	 * when they go out of scope, they automatically submit their timing data
	 * to the global Profiler instance.
	 *
	 * @note ProfilingSample cannot be heap-allocated to ensure correct
	 * RAII-based lifetime management.
	 */
	struct ProfilingSample {
		/**
		 * @brief Creates and starts a new profiling sample.
		 * @param in_name The name of the code block being profiled.
		 *
		 * The constructor captures the current time from Application::GetCurrentTime()
		 * and registers the sample’s metadata with the Profiler.
		 */
		ProfilingSample(std::string_view in_name);

		/**
		 * @brief Ends the profiling sample and submits it to the Profiler.
		 *
		 * The destructor records the end time, updates the scope depth, and
		 * automatically adds the completed sample to the Profiler’s list of samples.
		 */
		virtual ~ProfilingSample();

		/**
		 * @brief Move constructor.
		 * Transfers ownership of the profiling data and invalidates the source sample.
		 */
		ProfilingSample(ProfilingSample&& other) noexcept;

		/**
		 * @brief Move assignment operator.
		 * Transfers ownership of the profiling data and invalidates the source sample.
		 */
		ProfilingSample& operator=(ProfilingSample&& other) noexcept;

		/**
		 * @brief Copy constructor.
		 * Creates a shallow copy of the sample but marks it as inactive.
		 * This prevents accidental double-submission to the Profiler.
		 */
		ProfilingSample(const ProfilingSample&) noexcept;

		/**
		 * @brief Copy assignment operator.
		 * Copies data but deactivates the copied instance to preserve ownership semantics.
		 */
		ProfilingSample& operator=(const ProfilingSample&) noexcept;

		/**
		 * @brief Disallow dynamic allocation of ProfilingSample.
		 *
		 * ProfilingSample is designed for stack-only allocation to enforce
		 * proper lifetime management via RAII.
		 */
		void* operator new(size_t) = delete;
		void* operator new[](size_t) = delete;

		/** @brief Human-readable name of the profiled section. */
		std::string_view name;

		/** @brief Unique identifier assigned by the Profiler. */
		uint32_t id;

		/** @brief Time (in seconds) when the profiling sample started. */
		double start_time;

		/** @brief Time (in seconds) when the profiling sample ended. */
		double end_time;

		/** @brief Indicates whether the sample is active (i.e., has not been moved or invalidated). */
		bool active;

		/** @brief Nesting depth level of this sample (used for hierarchical profiling visualization). */
		uint32_t depth = 0;
	};

	/**
	 * @brief Singleton that collects and manages profiling samples.
	 *
	 * The Profiler provides a thread-safe container for all ProfilingSample
	 * instances recorded during a frame. It supports clearing all samples,
	 * generating new unique sample IDs, and exposing the current list of samples
	 * for rendering or export.
	 *
	 * The Profiler is designed for lightweight, per-frame performance measurement
	 * and visualization, such as in an on-screen debug overlay.
	 */
	class Profiler
	{
		friend struct ProfilingSample;

	public:
		/**
		 * @brief Returns the global Profiler instance.
		 * @return Reference to the singleton Profiler.
		 */
		static Profiler& Get();

		/**
		 * @brief Clears all currently stored profiling samples.
		 *
		 * This function should typically be called once per frame after
		 * rendering the profiling overlay. It also resets the internal
		 * sample ID counter.
		 */
		static void Clear();

		/**
		 * @brief Indicates whether the Profiler is currently clearing samples.
		 * @return True if the Profiler is clearing its data.
		 *
		 * Used internally to prevent ProfilingSample instances from
		 * submitting results while the buffer is being cleared.
		 */
		static bool IsClearing();

		/**
		 * @brief Returns a reference to the list of all recorded samples.
		 * @return Const reference to the current vector of ProfilingSamples.
		 */
		static const std::vector<ProfilingSample>& GetCurrentSamples();

		/**
		 * @brief Generates a new unique profiling sample ID.
		 * @return A sequentially incremented 32-bit ID.
		 */
		static uint32_t GetId();

		/** @brief Default destructor. */
		~Profiler() = default;

	private:
		/**
		 * @brief Adds a completed profiling sample to the internal sample list.
		 * @param sample The profiling sample to store.
		 */
		void AddSample(ProfilingSample&& sample);

		/** @brief Private constructor (singleton pattern). */
		Profiler() = default;

		Profiler(const Profiler&) = delete;
		Profiler& operator=(const Profiler&) = delete;
		Profiler(Profiler&&) = delete;
		Profiler& operator=(Profiler&&) = delete;

	private:
		/** @brief Flag indicating whether the Profiler is currently being cleared. */
		bool is_clearing = false;

		/** @brief Mutex to ensure thread-safe access to the sample list. */
		std::mutex mutex;

		/** @brief Collection of all ProfilingSamples recorded during the frame. */
		std::vector<ProfilingSample> samples;
	};

}



