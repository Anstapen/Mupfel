#pragma once
#include "Api/Events.h"
#include "ECS/Entity.h"
#include "Core/Application.h"
#include <functional>
#include <utility>

namespace Mupfel
{
    struct Timer
    {
        using FireEvent = std::function<void (Entity e)>;
        double duration = 0;
        bool repeating = false;
        bool expired = false;
        FireEvent fire = nullptr;

        void Arm(double new_duration)
        {
            duration = new_duration;
            expired = false;
        }

        template <EventType T>
        static Timer CreateTimer(double duration, bool repeating)
        {
            return {
                duration, repeating,
                [](Entity e) { Events::Post<T>(T(e)); }
            };
        }

    private:
        Timer() = default;

        Timer(double in_duration, bool in_repeating, FireEvent in_fire) :
            duration(in_duration), repeating(in_repeating), fire(std::move(in_fire))
        {
        }
    };
}
