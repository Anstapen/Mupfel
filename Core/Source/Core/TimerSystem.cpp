#include "TimerSystem.h"
#include "ECS/Components/Timer.h"
#include "Core/Application.h"
#include "Api/Entities.h"


void Mupfel::TimerSystem::Update(double elapsedTime)
{
    for (const auto &[e, t] : Entities::Each<Timer>())
    {
        if (t.expired)
        {
            continue;
        }

        t.duration -= elapsedTime;

        if (t.duration <= 0)
        {
            /* Timer has expired, fire the event */
            t.expired = false;
            t.fire(e);
        }
    }
}