#include "Profiler.h"
#include "Application.h"
#include <iostream>
#include <iomanip>

using namespace Mupfel;

/**
 * @brief Thread-local variable that tracks current nesting depth
 * of profiling samples per thread. Each nested ProfilingSample increases
 * this value; it is decremented when samples go out of scope.
 */
thread_local uint32_t scope = 0;

/**
 * @brief Global sample counter used to assign unique IDs to profiling samples.
 */
static uint32_t sample_id = 0;

ProfilingSample::ProfilingSample(std::string_view in_name) : name(in_name), active(true), depth(0), end_time(0.0f), id(0)
{
	start_time = Application::GetCurrentTime();
	id = Profiler::GetId();
	depth = scope;
	scope++;;
}

ProfilingSample::~ProfilingSample()
{
	if (!active)
	{
		return;
	}

	end_time = Application::GetCurrentTime();

	if (scope > 0)
	{
		scope--;
	}

	if (!Profiler::IsClearing())
	{
		Profiler::Get().AddSample(std::move(*this));
	}
	
}

Mupfel::ProfilingSample::ProfilingSample(ProfilingSample&& other) noexcept :
	name(other.name),
	start_time(other.start_time),
	end_time(other.end_time),
	active(other.active),
	depth(other.depth),
	id(other.id)
{
	other.active = false;
}

ProfilingSample& Mupfel::ProfilingSample::operator=(ProfilingSample&& other) noexcept
{
	if (this != &other) {
		name = other.name;
		end_time = other.end_time;
		start_time = other.start_time;
		active = other.active;
		depth = other.depth;
		id = other.id;
		other.active = false; 
	}
	return *this;
}

Mupfel::ProfilingSample::ProfilingSample(const ProfilingSample& other) noexcept :
	name(other.name),
	start_time(other.start_time),
	end_time(other.end_time),
	active(false),
	depth(other.depth),
	id(other.id)
{

}

ProfilingSample& Mupfel::ProfilingSample::operator=(const ProfilingSample& other) noexcept
{
	if (this != &other) {
		name = other.name;
		start_time = other.start_time;
		end_time = other.end_time;
		depth = other.depth;
		id = other.id;
		active = false;
	}
	return *this;
}

Profiler& Profiler::Get()
{
	static Profiler* inst = new Profiler();
	return *inst;
}

void Profiler::Clear() {
	std::scoped_lock lock(Get().mutex);
	Get().is_clearing = true;
	Get().samples.clear();
	Get().is_clearing = false;
	sample_id = 0;
}

bool Mupfel::Profiler::IsClearing()
{
	return Get().is_clearing;
}

const std::vector<ProfilingSample>& Mupfel::Profiler::GetCurrentSamples()
{
	return Get().samples;
}

uint32_t Mupfel::Profiler::GetId()
{
	std::scoped_lock lock(Get().mutex);
	uint32_t new_id = sample_id;
	sample_id++;
	return new_id;
}

void Mupfel::Profiler::AddSample(ProfilingSample&& sample)
{
	std::scoped_lock lock(mutex);
	samples.push_back(std::move(sample));
}
