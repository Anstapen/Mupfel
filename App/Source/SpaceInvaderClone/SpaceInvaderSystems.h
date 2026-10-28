#pragma once
#include "Mupfel.h"

class SpaceInvaderSystems
{
public:
    static void Update(double elapsedTime);
private:
    static void UpdateHits();
    static void UpdateDeaths();
    static void UpdateHP();
    static void UpdateCanons(double elapsedTime);
    static void DamageEntity(Mupfel::Entity e, uint32_t val);
};

