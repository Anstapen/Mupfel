#include "Mupfel.h"
#include "HelloWorldLayer.h"

int main()
{

	Mupfel::ApplicationSpecification app_spec;
	app_spec.name.insert(0, "My first Application");

	if (Mupfel::App::Init(app_spec))
	{
		Mupfel::App::PushLayer<HelloWorldLayer>();

		Mupfel::App::Run();
	}

	

	return 0;
}