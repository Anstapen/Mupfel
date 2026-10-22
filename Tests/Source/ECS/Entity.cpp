#include "ECS/Entity.h"
#include "catch_amalgamated.hpp"

TEST_CASE("Basic Entity System Tests", "[entity_basic]")
{
	Mupfel::EntityManager entity_manager;

	SECTION("First Entity should always be 1")
	{
		uint32_t entity_index = 1;
		REQUIRE(entity_manager.CreateEntity().Index() == entity_index);
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
