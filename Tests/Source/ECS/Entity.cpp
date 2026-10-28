#include "ECS/Entity.h"
#include "Accessors/EntityManagerAccess.h"
#include "Accessors/EntityAccess.h"
#include "catch_amalgamated.hpp"

using namespace Mupfel;
using EntAccess = Testing::EntityAccess;
using EntManagerAccess = Testing::EntityManagerAccess;

TEST_CASE("Basic Entity System Tests", "[entity_basic]")
{
	Mupfel::EntityManager entity_manager;

	SECTION("First Entity should always be 1")
	{
		const Entity e = entity_manager.CreateEntity();
	}

	SECTION("Test entity recycling")
	{
		Mupfel::Entity entity = entity_manager.CreateEntity();
		uint32_t	   first_entity_index = entity.Index();
		entity_manager.DestroyEntity(entity);
		entity = entity_manager.CreateEntity();
		uint32_t second_entity_index = entity.Index();
		/* Entity index should be recycled. */
		REQUIRE(first_entity_index == second_entity_index);
	}

	SECTION("Double deleting entities")
	{
		Mupfel::Entity entity = entity_manager.CreateEntity();
		uint32_t	   first_entity_index = entity.Index();

		/* Second destroy should be a no-op. */
		entity_manager.DestroyEntity(entity);
		entity_manager.DestroyEntity(entity);
		entity = entity_manager.CreateEntity();
		uint32_t second_entity_index = entity.Index();
		/* Entity index should be recycled. */
		REQUIRE(first_entity_index == second_entity_index);
	}
}
