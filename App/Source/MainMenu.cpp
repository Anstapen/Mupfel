#include "MainMenu.h"
#include "Mupfel.h"
#include "SceneSwitches.h"

using namespace Mupfel;

void MainMenu::OnInit() {}

void MainMenu::OnUpdate(double timestep) {}

void MainMenu::OnRender() {/* Draw a Button */
	if (UI::Button(70.0f, 50.0f, 136.0f, 53.0f, "Images/buttons/home.png") == 2)
	{
		Events::Post<SwitchToLevelEvent>({});
	}
}

void MainMenu::OnSwitchIn() {}

void MainMenu::OnSwitchOut() {}

void MainMenu::Serialize(const std::string& path) {}

void MainMenu::Deserialize(const std::string& path) {}
