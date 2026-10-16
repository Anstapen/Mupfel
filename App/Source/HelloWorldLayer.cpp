#include "HelloWorldLayer.h"
#include "SceneSwitches.h"
#include "Level.h"
#include "MainMenu.h"
#include "GravityTest.h"
#include "Systems/ProximityAnimationSystem.h"
#include "SpaceInvaderClone/Level.h"

using namespace Mupfel;

HelloWorldLayer::HelloWorldLayer() = default;

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
	scenes[AppScene::MAIN_MENU] = Scenes::Create<MainMenu>(def);
	def.name = "Dungeon";
	scenes[AppScene::DUNGEON_LEVEL] = Scenes::Create<Level>(def);
	def.name = "Space Invaders";
	scenes[AppScene::SPACE_INVADERS] = Scenes::Create<SpaceInvader::Level>(def);
	def.name = "GravityTest";
	def.gravity_x = 0.0f;
	def.gravity_y = -10.0f;
	scenes[AppScene::GRAV_TEST] = Scenes::Create<GravityTest>(def);

	Scenes::Switch(scenes[AppScene::MAIN_MENU]);

	Input::MapKey<SwitchSceneEvent>(Key::KEY_ESCAPE, KeyAction::RELEASED, SwitchSceneEvent(AppScene::MAIN_MENU));

}

void HelloWorldLayer::OnUpdate(double timestep)
{ 
	/* Check Events. There should not appear multiple of those in a frame. */
	if (Events::Pending<SwitchSceneEvent>() > 0)
	{
		if (const auto event = Events::Latest<SwitchSceneEvent>())
		{
			const auto it = scenes.find(event.value().new_scene);

			if (it != scenes.end())
			{
				Scenes::Switch(it->second);
			}
		}
	}

	ProximityAnimationSystem::Update(timestep);
}

void HelloWorldLayer::OnRender() {}

void HelloWorldLayer::ProcessEvents() {}
