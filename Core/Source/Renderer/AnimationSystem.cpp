#include "AnimationSystem.h"
#include "Core/Application.h"
#include "ECS/Components/Animation.h"
#include <algorithm>
#include <cstdint>

bool Mupfel::AnimationSystem::Init() { return true; }

void Mupfel::AnimationSystem::Update(double timestep)
{
	Mupfel::Registry& registry = Mupfel::Application::GetCurrentRegistry();
	for (auto [e, animation] : registry.view<Animation>())
	{
		if (animation.IsFinished())
		{
			continue;
		}

		animation.elapsed += static_cast<float>(timestep);

		if (!animation.repeating)
		{
			/*
			 * Check if the animation already went through.
			 * (elapsed * fps) / frameCount calculates how many percent of the animation already
			 * occured (with 1.0f being 100%).
			 */
			if (((animation.elapsed * animation.fps) / static_cast<float>(animation.frameCount)) > 1.0f)
			{
				/* Animation went through, just set the last frame */
				animation.currentFrame =
					animation.frameCount == 0 ? animation.firstFrame : animation.firstFrame + animation.frameCount - 1;
				animation.finished = true;
				continue;
			}
		}

		uint32_t step = static_cast<uint32_t>(animation.elapsed * animation.fps) % animation.frameCount;
		animation.currentFrame = animation.firstFrame + step;
	}
}

void Mupfel::AnimationSystem::Shutdown() {}
