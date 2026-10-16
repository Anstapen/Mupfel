#include "MainMenu.h"
#include "Mupfel.h"
#include "SceneSwitches.h"


void MainMenu::OnInit()
{
}

void MainMenu::OnUpdate(double timestep)
{
}

void MainMenu::OnRender()
{
    /* Draw a Button */
    if (Mupfel::UI::Button(50.0f, 50.0f, 150.0f, 50.0f, "Images/buttons/level.png") == 3)
    {
        logger->info("Switching to Level...");
        Mupfel::Events::Post<SwitchSceneEvent>(SwitchSceneEvent(AppScene::DUNGEON_LEVEL));
    }

    if (Mupfel::UI::Button(250.0f, 50.0f, 150.0f, 50.0f, "Images/buttons/grav.png") == 3)
    {
        logger->info("Switching to GravityTest...");
        Mupfel::Events::Post<SwitchSceneEvent>(SwitchSceneEvent(AppScene::GRAV_TEST));
    }

    if (Mupfel::UI::Button(450.0f, 50.0f, 150.0f, 50.0f, "Images/buttons/space_invaders.png") == 3)
    {
        logger->info("Switching to Space Invader Demo...");
        Mupfel::Events::Post<SwitchSceneEvent>(SwitchSceneEvent(AppScene::SPACE_INVADERS));
    }
}
