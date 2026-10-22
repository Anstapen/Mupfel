#include "Level.h"
#include "Invader.h"

using namespace Mupfel;

namespace SpaceInvader
{
    Level::Level(Mupfel::SceneHandle in_handle, const Mupfel::SceneDefinition& in_def) :
        Scene(in_handle, in_def), grid(18, 36)
    {
    }

    void Level::OnInit()
    {
        const ImageHandle texture = Images::LoadAnimated("Images/SpaceInvaders/simple_invader_animated.png", {1, 8}).
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
                Entities::AddComponent<Texture>(e, {.index = texture, .scale_x = 0.8f, .scale_y = 0.8f});
            }
        }
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
