#pragma once
#include "Mupfel.h"

class LightingTest : public Mupfel::Scene
{
public:
    LightingTest(Mupfel::SceneHandle in_handle, const Mupfel::SceneDefinition& def)
        : Scene(in_handle, def)
    {
    }

    void OnInit() final;
    void OnUpdate(double timestep) final;
    void OnRender() final;

private:
    void UpdateUserInputs();
    Mupfel::ImageHandle texture = INVALID_HANDLE;
    Mupfel::PRNG rnd;
};
