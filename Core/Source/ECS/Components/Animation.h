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

	Animation(uint32_t in_first_frame, uint32_t in_frame_count, float in_fps)
		: firstFrame(in_first_frame), frameCount(in_frame_count), fps(in_fps)
	{
	}

	uint32_t firstFrame = 0;
	uint32_t frameCount = 1;
	float	 fps = 10.0f;

private:
	float	 elapsed = 0.0f;
	uint32_t currentFrame = 0;
};
} // namespace Mupfel
