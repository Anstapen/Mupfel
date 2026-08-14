#include "HelloWorldLayer.h"
#include "SceneSwitches.h"
#include "Level.h"
#include "MainMenu.h"
#include "GravityTest.h"

using namespace Mupfel;

HelloWorldLayer::HelloWorldLayer() {}

void HelloWorldLayer::OnInit()
{
	/* Create a Scene */
	mainMenu = Scenes::Create<MainMenu>("MainMenu");
	level = Scenes::Create<Level>("Dungeon");
	gravityTest = Scenes::Create<GravityTest>("GravityTest");

	/* We are starting with the gravityTest. */
	Scenes::Switch(gravityTest);
}

void HelloWorldLayer::OnUpdate(double timestep)
{ 
	/* Check Events. There should not appear multiple of those in a frame. */
	if (Events::Pending<SwitchToMainMenuEvent>() > 0)
	{
		Scenes::Switch(mainMenu);
	}
	if (Events::Pending<SwitchToLevelEvent>() > 0)
	{
		Scenes::Switch(level);
	}
	if (Events::Pending<SwitchToGravityTestEvent>() > 0)
	{
		Scenes::Switch(gravityTest);
	}
}

void HelloWorldLayer::OnRender() {}

void HelloWorldLayer::ProcessEvents() {}
