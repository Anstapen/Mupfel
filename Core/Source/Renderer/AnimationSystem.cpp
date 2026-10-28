#include "AnimationSystem.h"
#include "ECS/Components/Animation.h"
#include "Api/Events.h"
#include "Core/CoreEvents.h"
#include <algorithm>
#include <cstdint>

bool Mupfel::AnimationSystem::Init() { return true; }

void Mupfel::AnimationSystem::Update(double timestep)
{
    Registry& registry = Application::GetCurrentRegistry();
    for (auto [e, animation] : registry.Each<Animation>())
    {
        if (animation.IsFinished())
        {
            continue;
        }

        animation.elapsed += static_cast<float>(timestep);

        if (!IsAnimationFinished(animation) || animation.GetCurrentClip().repeating)
        {
            animation.SetNextFrame();
            continue;
        }

        /* The animation is finshed this frame */
        /* Publish an event. */
        Events::PostNow<AnimationEndedEvent>({e});

        /* If the animation has another clip queued, we just consume it and move on. */
        if (animation.HasClipQueued())
        {
            animation.StartClip(animation.nextQueuedClip);
            animation.nextQueuedClip = -1;
            continue;
        }

        /* The animation is done and has nothing queued, finish it. */
        animation.Finish();

    }
}

void Mupfel::AnimationSystem::Shutdown()
{
}

bool Mupfel::AnimationSystem::IsAnimationFinished(const Animation& animation)
{
    const auto& currentClip = animation.GetCurrentClip();
    return (animation.elapsed * currentClip.fps) / static_cast<float>(currentClip.frameCount) > 1.0f;
}
