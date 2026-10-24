#include "Player.h"
#include "Types.h"
#include "Imager.h"
#include <string_view>
#include "MovementEvent.h"
#include "Components/UserMovable.h"

using namespace Mupfel;

void Player::Init()
{
    logger = Logger::Create("Player");

    Imager::LoadAnimated("PlayerNaked", "Images/spritesheet.png", {.rows = 1, .columns = 8});

    animations["idle_front"] = {0, 8, 8.0f, 0.0f, true, true};
    animations["idle_back"] = {0, 8, 8.0f, 0.0f, true, true};
    animations["idle_right"] = {0, 8, 8.0f, 0.0f, true, true};
    animations["idle_left"] = {0, 8, 8.0f, 0.0f, true, true};

    const Entity e = Entities::Create();
    Transform p;
    p.pos_z = 0.1f;

    Entities::AddComponent<Transform>(e, p);

    Texture tex;
    tex.scale_x = 2.0f;
    tex.scale_y = 2.0f;
    tex.index = Imager::Get("PlayerNaked");
    Entities::AddComponent<Texture>(e, tex);

    current_anim = "idle_front";
    Entities::AddComponent<Animation>(e, animations.at(current_anim));

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
        std::string_view wanted = "idle_front";

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
            wanted = "idle_right";
        }
        if (velocity_x < 0.0f)
        {
            wanted = "idle_left";
        }
        if (velocity_y > 0.0f)
        {
            wanted = "idle_back";
        }
        if (velocity_y < 0.0f)
        {
            wanted = "idle_front";
        }


        if (!wanted.empty() && wanted != current_anim)
        {
            current_anim = wanted;
            Entities::GetComponent<Animation>(ent) = animations.at(current_anim);
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
