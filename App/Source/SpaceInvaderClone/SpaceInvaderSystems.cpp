#include "SpaceInvaderSystems.h"

#include "SpaceInvaderEntityBehavior.h"
#include "Components/HealthPoints.h"
#include "Components/DamageOnContact.h"
#include "Components/Bullet.h"
#include "Components/OnHit.h"
#include "Components/OnDeath.h"
#include "Components/Canon.h"
#include "Events/InvaderDestroyedEvent.h"

using namespace Mupfel;

void SpaceInvaderSystems::Update(double elapsedTime)
{
    UpdateHits();
    UpdateCanons(elapsedTime);
    UpdateHP();
    UpdateDeaths();
}

void SpaceInvaderSystems::UpdateHits()
{
    for (const auto& event : Events::Get<SensorEnteredEvent>())
    {
        if (const auto* damageOnContact = Entities::TryGetComponent<DamageOnContact>(event.visitor))
        {
            DamageEntity(event.sensor, damageOnContact->value);
        }

        if (const auto *onHit = Entities::TryGetComponent<OnHit>(event.sensor))
        {
            onHit->behavior(event.sensor);
        }
    }
}

void SpaceInvaderSystems::UpdateDeaths()
{
    for (const auto& event : Events::Get<InvaderDestroyedEvent>())
    {
        if (const auto *onDeath = Entities::TryGetComponent<OnDeath>(event.e))
        {
            onDeath->behavior(event.e);
        } else
        {
            Entities::Destroy(event.e);
        }
    }
}

void SpaceInvaderSystems::DamageEntity(Entity e, uint32_t val)
{
    if (!Entities::HasComponent<HealthPoints>(e))
    {
        return;
    }

    auto& healthPoints = Entities::GetComponent<HealthPoints>(e);

    if (healthPoints.value == 0)
    {
        return;
    }

    const uint32_t original_hp = healthPoints.value;
    healthPoints.value -= std::min<uint32_t>(healthPoints.value, val);

    if (original_hp > healthPoints.value && healthPoints.onHPLoss)
    {
        healthPoints.onHPLoss(e, healthPoints);
    }
}


void SpaceInvaderSystems::UpdateHP()
{
}

void SpaceInvaderSystems::UpdateCanons(double elapsedTime)
{
    for (auto [e, c] : Entities::Each<Canon>())
    {
        c.cooldownValue += elapsedTime;

        if (c.cooldownValue > (1.0 / c.fireRate))
        {
            c.cooldownValue -= (1.0 / c.fireRate);

            if (!Entities::HasComponent<Transform>(e))
            {
                return;
            }

            /* Shoot a projectile */
            const Entity proj = Entities::Create();

            const auto& pos = Entities::GetComponent<Transform>(e);

            Entities::AddComponent<Transform>(proj, {.pos_x = pos.pos_x, .pos_y = pos.pos_y, .pos_z = pos.pos_z});
            const ImageHandle proj_texture = Images::LoadAnimated("Images/SpaceInvaders/blue_laser_animated.png",
                                                                  {.rows = 1, .columns = 8}).
                value_or(Scene::INVALID_HANDLE);
            Entities::AddComponent<Texture>(proj, {.index = proj_texture});
            Entities::AddComponent<Body>(proj, {.type = BodyType::Kinematic, .velocity_x = 0.0f, .velocity_y = 15.0f});
            Animation a;
            a.clips[a.idleClip] = {.firstFrame = 0, .frameCount = 8, .fps = 16.0f, .repeating = true};
            Entities::AddComponent<Animation>(proj, a);

            Sensor s;
            s.SetCircle(0.2f);
            s.offset_y = 0.2f;
            s.report_events = true;
            Entities::AddComponent<Sensor>(proj, s);
            Entities::AddComponent<DamageOnContact>(proj, {1});
            Entities::AddComponent<Bullet>(proj, {.piercesLeft = 0});
            Entities::AddComponent<OnHit>(proj, {LaserOnHit});
        }
    }
}
