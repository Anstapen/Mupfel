#include "InvaderSystem.h"
#include "Invader.h"
#include "Mupfel.h"

using namespace Mupfel;

void InvaderSystem::Update(double timestep, const SpatialGrid &grid)
{
    /* Check if another tick is due. */
    if (!UpdateTime(timestep))
    {
        return;
    }

    for (auto [e, i, t] : Entities::Each<Invader, Transform>())
    {
        /* Update the movement direction */
        grid.AdvancePosition(i.pos);
        t.pos_x = i.pos.x;
        t.pos_y = i.pos.y;
        Application::SetTransform(e,t);
    }
}

void InvaderSystem::SetTPS(uint32_t tps)
{
    if (tps == 0)
    {
        return;
    }

    ticksPerSecond = tps;
    tickPeriod = 1.0 / static_cast<double>(ticksPerSecond);
}

bool InvaderSystem::UpdateTime(double timestep)
{
    currentTime += timestep;

    if (currentTime >= tickPeriod)
    {
        currentTime -= tickPeriod;
        return true;
    }

    return false;
}
