#pragma once
#include <cstdint>

namespace Mupfel
{
class AnimationSystem;
class ECSRenderer;

struct Animation
{
	friend class AnimationSystem;
	friend class ECSRenderer;

	Animation() = default;

	Animation(
		uint32_t in_first_frame,
		uint32_t in_frame_count,
		float	 in_fps,
		float	 elapsed_time = 0.0f,
		bool	 in_repeating = true,
		bool	 auto_start = false)
		: firstFrame(in_first_frame), frameCount(in_frame_count), fps(in_fps), elapsed(elapsed_time),
		  repeating(in_repeating), finished(!auto_start)
	{
	}

	inline void Reset()
	{
		elapsed = 0.0f;
		currentFrame = firstFrame;
		finished = false;
	}

	inline bool IsFinished() const { return finished; }

	uint32_t firstFrame = 0;
	uint32_t frameCount = 1;
	float	 fps = 10.0f;
	bool	 repeating = true;

private:
	float	 elapsed = 0.0f;
	uint32_t currentFrame = 0;
	bool	 finished = false;
};
} // namespace Mupfel
