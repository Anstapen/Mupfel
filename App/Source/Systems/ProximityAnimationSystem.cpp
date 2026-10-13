#include "ProximityAnimationSystem.h"
#include "Mupfel.h"
#include "Components/ProximityAnimator.h"

using namespace Mupfel;

void ProximityAnimationSystem::Update(double elapsedTime)
{
    for (const auto &event : Events::Get<SensorEnteredEvent>())
    {
        if (Entities::HasComponent<ProximityAnimator>(event.sensor))
        {
            auto &pa = Entities::GetComponent<ProximityAnimator>(event.sensor);
            if (pa.current_visitors++ == 0)
            {
                pa.pending = pa.on_enter;
            }
        }
    }

    for (const auto &event : Events::Get<SensorExitedEvent>())
    {
        if (Entities::HasComponent<ProximityAnimator>(event.sensor))
        {
            auto &pa = Entities::GetComponent<ProximityAnimator>(event.sensor);
            if (--pa.current_visitors == 0)
            {
                pa.pending = pa.on_exit;
            }
        }
    }

    /* Iterate over entities and update the animation, if needed */
    for (auto [entity, pa, a] : Entities::Each<ProximityAnimator, Animation>())
    {
        if (pa.pending && a.IsFinished())
        {
            a = pa.pending.value();
            pa.pending.reset();
        }
    }
}
