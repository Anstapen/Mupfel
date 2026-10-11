#include "MainMenu.h"
#include "Mupfel.h"
#include "SceneSwitches.h"

using namespace Mupfel;

void MainMenu::OnInit() {}

void MainMenu::OnUpdate(double timestep) {}

void MainMenu::OnRender() {/* Draw a Button */
	if (UI::Button(50.0f, 50.0f, 150.0f, 50.0f, "Images/buttons/level.png") == 3)
	{
		logger->info("Switching to Level...");
		Events::Post<SwitchToLevelEvent>({});
	}

	if (UI::Button(250.0f, 50.0f, 150.0f, 50.0f, "Images/buttons/grav.png") == 3)
	{
		logger->info("Switching to GravityTest...");
		Events::Post<SwitchToGravityTestEvent>({});
	}
}
