#include "Core/Application.h"
#include "HelloWorldLayer.h"

int main()
{

	Mupfel::ApplicationSpecification app_spec;
	app_spec.name.insert(0, "My first Application");
	app_spec.physics_strategy = Mupfel::ComputationStrategy::GPU;

	Mupfel::Application& app = Mupfel::Application::Get();

	if (app.Init(app_spec))
	{
		app.PushLayer<HelloWorldLayer>();

		app.Run();
	}

	

	return 0;
}