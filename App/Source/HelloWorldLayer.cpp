#include "HelloWorldLayer.h"
#include "SceneSwitches.h"
#include "Level.h"
#include "MainMenu.h"
#include "GravityTest.h"

using namespace Mupfel;

HelloWorldLayer::HelloWorldLayer() {}

void HelloWorldLayer::OnInit()
{
	/* Try to load a resource pack. */
	mngr = std::make_shared<ResourceManager>();
	if (!mngr->Load("Resources/resource.pack"))
	{
		logger->warn("Unable to load resource file!");
	}

	/* Create a Scene */
	SceneDefinition def;
	def.name = "MainMenu";
	mainMenu = Scenes::Create<MainMenu>(def);
	def.name = "Dungeon";
	level = Scenes::Create<Level>(def, mngr);
	def.name = "GravityTest";
	def.gravity_x = 0.0f;
	def.gravity_y = -10.0f;
	gravityTest = Scenes::Create<GravityTest>(def);

	/* We are starting with the gravityTest. */
	Scenes::Switch(level);

	/* Create a logger object. */
	logger = Logger::Create("HelloWorldLayer");


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
