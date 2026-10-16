#pragma once
#include "Mupfel.h"

namespace SpaceInvader
{
    class Level : public Mupfel::Scene
    {
    public:
        Level(Mupfel::SceneHandle in_handle, const Mupfel::SceneDefinition& in_def);
        void OnInit() final;
        void OnUpdate(double timestep) final;
        void OnRender() final;
    };
}
