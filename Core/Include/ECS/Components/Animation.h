#pragma once
#include <cstdint>
#include <array>
#include "json_fwd.hpp"


namespace Mupfel
{
    class AnimationSystem;
    class ECSRenderer;

    struct Animation
    {
        friend class AnimationSystem;
        friend class ECSRenderer;


        struct Clip
        {
            uint32_t firstFrame = 0;
            uint32_t frameCount = 1;
            float fps = 10.0f;
            bool repeating = true;
        };

        static constexpr uint32_t maxClips = 8;
        using ClipArray = std::array<Clip, maxClips>;

        Animation() = default;

        Animation(
            const ClipArray& in_clips,
            uint32_t in_idle_clip)
            : clips(in_clips), idleClip(in_idle_clip)
        {
        }

        void StartClip(uint32_t id, float elapsedTime = 0.0f)
        {
            elapsed = elapsedTime;
            finished = false;
            currentClip = id;
            currentFrame = clips[id].firstFrame;
        }

        [[nodiscard]] bool HasClipQueued() const
        {
            return nextQueuedClip > 0;
        }

        void SetNextFrame()
        {
            const auto& clip = GetCurrentClip();
            const uint32_t step = static_cast<uint32_t>(elapsed * clip.fps) % std::max<uint32_t>(
                clip.frameCount, 1);
            currentFrame = clip.firstFrame + step;
        }

        void Finish()
        {
            const auto& clip = GetCurrentClip();
            currentFrame = clip.frameCount == 0
                               ? clip.firstFrame
                               : clip.firstFrame + clip.frameCount - 1;
            finished = true;
        }

        Clip& GetCurrentClip()
        {
            return clips[currentClip];
        }

        [[nodiscard]] const Clip& GetCurrentClip() const
        {
            return clips[currentClip];
        }

        void QueueClip(uint32_t id)
        {
            if (id < maxClips)
            {
                nextQueuedClip = static_cast<int32_t>(id);
            }
        }

        void Reset()
        {
            elapsed = 0.0f;
            currentFrame = clips[idleClip].firstFrame;
            finished = false;
        }

        [[nodiscard]] bool IsFinished() const { return finished; }

        void BuildFromJSON(const nlohmann::json& source);

        ClipArray clips;
        uint32_t idleClip = 0;
        uint32_t currentClip = 0;
        int32_t nextQueuedClip = -1;

    private:
        float elapsed = 0.0f;
        uint32_t currentFrame = 0;
        bool finished = false;
    };
} // namespace Mupfel
