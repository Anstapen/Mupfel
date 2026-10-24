#include "Level.h"
#include "Invader.h"
#include "Mupfel.h"
#include "Components/UserMovable.h"

using namespace Mupfel;

namespace SpaceInvader
{
    Level::Level(Mupfel::SceneHandle in_handle, const Mupfel::SceneDefinition& in_def) :
        Scene(in_handle, in_def), grid(18, 36)
    {
    }

    void Level::OnInit()
    {
        const ImageHandle invader_texture = Images::LoadAnimated("Images/SpaceInvaders/simple_invader_animated.png",
                                                                 {.rows = 1, .columns = 8}).
            value_or(INVALID_HANDLE);
        camera.target_x = std::floor(static_cast<float>(grid.num_columns) / 2.0f);
        camera.target_y = std::floor(static_cast<float>(grid.num_rows) / 2.0f);
        /* Grid test */
        for (uint32_t row = grid.num_rows - 1; row > grid.num_rows - 5; --row)
        {
            for (uint32_t column = 6; column < grid.num_columns - 6; ++column)
            {
                const Entity e = Entities::Create();
                const auto pos_x = static_cast<float>(column);
                const auto pos_y = static_cast<float>(row);
                const SpatialGrid::GridPoint p{.x = column, .y = row};
                Entities::AddComponent<Invader>(e, {p});
                Entities::AddComponent<Transform>(e, {.pos_x = pos_x, .pos_y = pos_y});
                Entities::AddComponent<Texture>(e, {.index = invader_texture, .scale_x = 0.8f, .scale_y = 0.8f});
            }
        }

        /* Place spaceship. */
        const Entity spaceship = Entities::Create();
        const ImageHandle spaceship_texture = Images::LoadAnimated("Images/SpaceInvaders/blue_spaceship_animated.png",
                                                                   {.rows = 1, .columns = 8}).
            value_or(INVALID_HANDLE);
        Entities::AddComponent<Transform>(spaceship, {.pos_x = 18, .pos_y = 0});
        Entities::AddComponent<Texture>(spaceship, {.index = spaceship_texture, .scale_x = 0.8f, .scale_y = 0.8f});
        Entities::AddComponent<Body>(spaceship, {.type = BodyType::Kinematic});

        const UserMovable::Update func = [](Mupfel::Entity ent, const MovementEvent& evt)
        {
            if (!Entities::HasComponent<Body>(ent))
            {
                /* Entity does not have a body component, nothing to do... */
                return;
            }
            auto& body = Entities::GetComponent<Body>(ent);
            const float direction = (evt.started) ? 1.0f : -1.0f;

            switch (evt.dir)
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


            Application::SetMovement(ent, body.velocity_x, body.velocity_y, 0.0f);
        };

        Entities::AddComponent<UserMovable>(spaceship, {func});
        Entities::AddComponent<Animation>(spaceship, {0, 8, 16.0f, 0, true, true});
    }

    void Level::OnUpdate(double timestep)
    {
        (void)timestep;
        invaderSystem.Update(timestep, grid);
    }

    void Level::OnRender()
    {
    }
}
