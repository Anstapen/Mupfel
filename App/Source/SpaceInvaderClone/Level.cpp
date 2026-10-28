#include "Level.h"
#include "Invader.h"
#include "Mupfel.h"
#include "Components/UserMovable.h"
#include "Components/Canon.h"
#include "Components/HealthPoints.h"
#include "Components/OnHit.h"
#include "SpaceInvaderSystems.h"
#include "SpaceInvaderEntityBehavior.h"

using namespace Mupfel;

namespace SpaceInvader
{
    Level::Level(SceneHandle in_handle, const SceneDefinition& in_def) :
        Scene(in_handle, in_def), grid(18, 36)
    {
    }

    void Level::OnInit()
    {
        const ImageHandle invader_texture = Images::LoadAnimated("Images/SpaceInvaders/simple_invader_animated.png",
                                                                 {.rows = 3, .columns = 8}).
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
                Entities::AddComponent<Body>(e, {.type = BodyType::Kinematic});
                Sensor s;
                s.SetCircle(0.35f);
                s.report_events = true;
                Entities::AddComponent<Sensor>(e, s);

                Animation a;
                a.clips[1] = {.firstFrame = 16, .frameCount = 8, .fps = 16.0f, .repeating = false};
                a.clips[2] = {.firstFrame = 8, .frameCount = 8, .fps = 16.0f, .repeating = false};
                a.clips[3] = {.firstFrame = 0, .frameCount = 8, .fps = 16.0f, .repeating = false};
                Entities::AddComponent<Animation>(e, a);
                Entities::AddComponent<HealthPoints>(e, {.value = 3, .onHPLoss = InvaderOnHPLoss});
                Entities::AddComponent<OnHit>(e, {InvaderOnHit});
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
        Entities::AddComponent<Canon>(spaceship, {.fireRate = 5.0f});

        Entities::AddComponent<UserMovable>(spaceship, {SpaceShipMovement});
        Animation a;
        a.clips[a.idleClip] = {.firstFrame = 0, .frameCount = 8, .fps = 16.0f, .repeating = true};
        Entities::AddComponent<Animation>(spaceship, a);
    }

    void Level::OnUpdate(double timestep)
    {
        (void)timestep;
        invaderSystem.Update(timestep, grid);
        SpaceInvaderSystems::Update(timestep);
    }

    void Level::OnRender()
    {
    }
}
