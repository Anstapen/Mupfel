#include "Player.h"
#include "Types.h"
#include "Imager.h"
#include "Events/UserInputEvent.h"
#include "Components/UserMovable.h"

using namespace Mupfel;

namespace
{
    enum PlayerDirections : uint8_t
    {
        FRONT = 0,
        BACK,
        RIGHT,
        LEFT
    };
}

void Player::Init()
{
    logger = Logger::Create("Player");

    Imager::LoadAnimated("PlayerNaked", "Images/spritesheet.png", {.rows = 1, .columns = 8});

    const Entity e = Entities::Create();
    Transform p;
    p.pos_z = 0.1f;

    Entities::AddComponent<Transform>(e, p);

    Texture tex;
    tex.scale_x = 2.0f;
    tex.scale_y = 2.0f;
    tex.index = Imager::Get("PlayerNaked");
    Entities::AddComponent<Texture>(e, tex);

    Animation a;
    Animation::Clip clip{.firstFrame = 0, .frameCount = 8, .fps = 8.0f, .repeating = true};
    a.clips[FRONT] = clip;
    a.clips[BACK] = clip;
    a.clips[RIGHT] = clip;
    a.clips[LEFT] = clip;

    Entities::AddComponent<Animation>(e, a);

    Collider c;
    c.SetBox(1.0f, 0.5f);
    c.SetCapsule(-0.3f, 0.0f, 0.3f, 0.0f, 0.2f);
    c.offset_y = -0.75;
    c.report_contacts = true;
    c.report_hit_events = true;
    c.report_sensor_events = true;
    c.category = ColliderType::Player;

    Entities::AddComponent<Collider>(e, c);

    Body b;
    b.type = BodyType::Dynamic;
    b.fixed_rotation = true;

    Entities::AddComponent<Body>(e, b);

    const UserMovable::Update func = [this](Mupfel::Entity ent, const MovementEvent& evt)
    {
        uint32_t wanted = FRONT;

        const float direction = (evt.started) ? 1.0f : -1.0f;

        switch (evt.dir)
        {
        case MovementEvent::Direction::FORWARD:
            velocity_y += 1.0f * direction;
            break;
        case MovementEvent::Direction::BACKWARD:
            velocity_y -= 1.0f * direction;
            break;
        case MovementEvent::Direction::LEFT:
            velocity_x -= 1.0f * direction;
            break;
        case MovementEvent::Direction::RIGHT:
            velocity_x += 1.0f * direction;
            break;
        case MovementEvent::Direction::NONE:
            break;
        }

        if (velocity_x > 0.0f)
        {
            wanted = RIGHT;
        }
        if (velocity_x < 0.0f)
        {
            wanted = LEFT;
        }
        if (velocity_y > 0.0f)
        {
            wanted = BACK;
        }
        if (velocity_y < 0.0f)
        {
            wanted = FRONT;
        }

        auto &anim = Entities::GetComponent<Animation>(ent);

        if (wanted != anim.currentClip)
        {
            anim.StartClip(wanted);
        }

        float vel_x = velocity_x;
        float vel_y = velocity_y;

        /* Do the cheap calculation first */
        const float magnitude_sqrd = powf(vel_x, 2) + powf(vel_y, 2);

        /* For the 0.0f case, we can test for inequality. */
        if (magnitude_sqrd != 0.0f)
        {
            const float magnitude = sqrtf(magnitude_sqrd);
            vel_x /= magnitude;
            vel_y /= magnitude;
        }

        Application::SetMovement(ent, vel_x * movement_speed, vel_y * movement_speed, 0.0f);
    };

    Entities::AddComponent<UserMovable>(e, {func});
}
