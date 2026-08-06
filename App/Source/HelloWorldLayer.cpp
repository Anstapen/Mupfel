#include "HelloWorldLayer.h"
#include "Core/Application.h"
#include "Core/Event.h"
#include "Core/Profiler.h"
#include "ECS/View.h"

#include "EditorLayer.h"
#include "Level.h"

using namespace Mupfel;

class SceneChangedEvent : public Mupfel::Event
{
public:
	SceneChangedEvent() {};
};

HelloWorldLayer::HelloWorldLayer() {}

void HelloWorldLayer::OnInit()
{

	/* Create a Scene */
	level = Application::CreateScene<Level>("Dungeon");

	Mupfel::InputManager& input_manager = Mupfel::Application::GetCurrentInputManager();
	input_manager.MapKeyboardButton<SceneChangedEvent>(Mupfel::Key::KEY_N, Mupfel::KeyAction::PRESSED, {});

}

void HelloWorldLayer::OnUpdate(double timestep)
{
	
	Mupfel::EventSystem& evt_system = Mupfel::Application::GetCurrentEventSystem();
	for (auto& event : evt_system.GetEvents<SceneChangedEvent>())
	{
		/* Switch the scene */
		if (Application::GetCurrentSceneHandle() == 0)
		{
			Mupfel::Application::QueueSceneSwitch(level);
		}
		else
		{
			Mupfel::Application::QueueSceneSwitch(0);
		}
		
	}
}

void HelloWorldLayer::OnRender() {}

void HelloWorldLayer::ProcessEvents() {}
