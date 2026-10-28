#include "SpaceInvaderEntityBehavior.h"
#include "Mupfel.h"
#include "Events/InvaderDestroyedEvent.h"
#include "Components/Bullet.h"

using namespace Mupfel;

void SpaceShipMovement(Entity e, const MovementEvent& event)
{
    if (!Entities::HasComponent<Body>(e))
    {
        /* Entity does not have a body component, nothing to do... */
        return;
    }
    auto& body = Entities::GetComponent<Body>(e);
    const float direction = (event.started) ? 1.0f : -1.0f;

    switch (event.dir)
    {
    case MovementEvent::Direction::LEFT:
        body.velocity_x -= 5.0f * direction;
        break;
    case MovementEvent::Direction::RIGHT:
        body.velocity_x += 5.0f * direction;
        break;
    default:
        break;
    }


    Application::SetMovement(e, body.velocity_x, body.velocity_y, 0.0f);
}

void InvaderOnHit(Entity e)
{
}

void InvaderOnHPLoss(Entity e, const HealthPoints &hp)
{
    uint32_t animation_index = 1;

    if (hp.value == 0)
    {
        /* Invader is dying. */
        /* Start a timer that destroys the invader. */
        const Timer t = Timer::CreateTimer<InvaderDestroyedEvent>(1, false);
        Entities::AddComponent<Timer>(e, t);

        /* Remove the sensor, we should not be hittable anymore. */
        Entities::RemoveComponent<Sensor>(e);

        animation_index = 2;
    }

    if (auto* animation = Entities::TryGetComponent<Animation>(e))
    {
        animation->StartClip(animation_index);
    }
}

void LaserOnHit(Entity e)
{
    if (auto* bullet = Entities::TryGetComponent<Bullet>(e))
    {
        bullet->piercesLeft -= (bullet->piercesLeft > 0) ? 1 : 0;

        if (bullet->piercesLeft == 0)
        {
            Entities::Destroy(e);
        }
    }
}

