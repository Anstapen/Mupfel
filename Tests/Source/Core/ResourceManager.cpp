#include "../../../Core/Include/Core/ResourceManager.h"
#include "catch_amalgamated.hpp"
#include <filesystem>

TEST_CASE("Basic Resource Manager Test")
{
	/* Create the resource file */
	Mupfel::ResourceManager resource_manager;
	if (!std::filesystem::exists("test.res"))
	{
		resource_manager.Store("test.res", "");
	}
	else
	{
		resource_manager.Load("test.res", "");
	}
}
