#include "Level.h"

using namespace Mupfel;

namespace SpaceInvader
{
    Level::Level(Mupfel::SceneHandle in_handle, const Mupfel::SceneDefinition& in_def) :
        Scene(in_handle, in_def)
    {
    }

    void Level::OnInit()
    {
    }

    void Level::OnUpdate(double timestep)
    {
        (void)timestep;
    }

    void Level::OnRender()
    {
    }
}
