#include "ECS/Entity.h"
#include "catch_amalgamated.hpp"
#include <limits>
#include "Random.h"

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

TEST_CASE("Entity Limit Tests", "[entity_limit]")
{
	Mupfel::EntityManager entity_manager;

	SECTION("Creating all entities")
	{
		for (uint32_t i = 0; i < std::numeric_limits<uint32_t>::max() - 1; i++)
		{
			entity_manager.CreateEntity();
		}

		/* One last entity should be possible. */
		REQUIRE(entity_manager.CreateEntity().Index() == std::numeric_limits<uint32_t>::max());

		/* The internal entity counter overflows. */
		REQUIRE(entity_manager.CreateEntity().Index() == 0);
	}

	/* TODO: create some random deletetion tests. */
}
