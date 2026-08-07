#include "HelloWorldLayer.h"
#include "SceneSwitches.h"
#include "Level.h"
#include "MainMenu.h"

using namespace Mupfel;

HelloWorldLayer::HelloWorldLayer() {}

void HelloWorldLayer::OnInit()
{
	/* Create a Scene */
	mainMenu = Scenes::Create<MainMenu>("MainMenu");
	level = Scenes::Create<Level>("Dungeon");
}

void HelloWorldLayer::OnUpdate(double timestep)
{ 
	/* Check Events */
	if (Events::Pending<SwitchToMainMenuEvent>() > 0)
	{
		Scenes::Switch(mainMenu);
	}
	else if (Events::Pending<SwitchToLevelEvent>() > 0)
	{
		Scenes::Switch(level);
	}
}

void HelloWorldLayer::OnRender() {}

void HelloWorldLayer::ProcessEvents() {}
