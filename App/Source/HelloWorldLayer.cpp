#include "HelloWorldLayer.h"
#include "SceneSwitches.h"
#include "Level.h"
#include "MainMenu.h"
#include "GravityTest.h"
#include "Systems/ProximityAnimationSystem.h"

using namespace Mupfel;

HelloWorldLayer::HelloWorldLayer() {}

void HelloWorldLayer::OnInit()
{
	/* Create a logger object. */
	logger = Logger::Create("HelloWorldLayer");

	/* Try to load a resource pack. */
	mngr = std::make_shared<ResourceManager>();
	if (!mngr->Load("Resources/resource.pack"))
	{
		logger->warn("Unable to load resource file!");
	} else
	{
		/* set it as the global resource manager. */
		Application::SetGlobalResourceManager(mngr);
	}


	/* Create a Scene */
	SceneDefinition def;
	def.name = "MainMenu";
	mainMenu = Scenes::Create<MainMenu>(def);
	def.name = "Dungeon";
	level = Scenes::Create<Level>(def);
	def.name = "GravityTest";
	def.gravity_x = 0.0f;
	def.gravity_y = -10.0f;
	gravityTest = Scenes::Create<GravityTest>(def);

	Scenes::Switch(level);

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

	ProximityAnimationSystem::Update(timestep);
}

void HelloWorldLayer::OnRender() {}

void HelloWorldLayer::ProcessEvents() {}
