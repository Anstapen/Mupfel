#pragma once
#include "Core/Event.h"
#include <cstdint>

enum class AppScene : uint8_t
{
    INVALID,
    MAIN_MENU,
    DUNGEON_LEVEL,
    SPACE_INVADERS,
    GRAV_TEST
};

class SwitchSceneEvent : public Mupfel::Event
{
public:
    explicit SwitchSceneEvent(AppScene in_scene = AppScene::INVALID) : new_scene(in_scene)
    {
    };
    AppScene new_scene;
};
