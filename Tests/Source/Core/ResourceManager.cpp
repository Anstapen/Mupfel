#include "catch_amalgamated.hpp"
#include <filesystem>
#include "Resource/ResourceManager.h"

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
