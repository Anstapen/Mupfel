#include "Core/Application.h"
#include "HelloWorldLayer.h"

#include <iostream>
#include <thread>

int main()
{
	Mupfel::ApplicationSpecification app_spec;
	app_spec.name.insert(0, "My first Application");

	std::cout << "Hardware Threads: " << std::thread::hardware_concurrency() << std::endl;

	Mupfel::Application& app = Mupfel::Application::Get();

	if (app.Init(app_spec))
	{
		app.PushLayer<HelloWorldLayer>();

		app.Run();
	}

	return 0;
}