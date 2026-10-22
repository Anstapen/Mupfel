/**
 * This file attempts to implement tests for the ComponentArray class.
 */
#include "catch_amalgamated.hpp"
#include "ECS/ComponentArray.h"
#include "ECS/Entity.h"
#include "Accessors/ComponentArrayAccess.h"
#include "Accessors/EntityAccess.h"
#include "Core/PRNG.h"
#include <ranges>

/* Anonymous namespace to restrict those component definitions to this compilation unit. */
namespace
{
    struct Comp1
    {
        uint32_t a;
    };
}

using namespace Mupfel;
using CompAccess = Testing::ComponentArrayAccess;
using EntAccess = Testing::EntityAccess;

template <ComponentType T>
static void CheckComponentIntegrity(const ComponentArray<T>& array, const Entity& e)
{
    auto& sparse = CompAccess::Sparse(array);
    auto& dense = CompAccess::Dense(array);
    auto& component = CompAccess::Components(array);

    REQUIRE(sparse.size() > e.Index());
    const size_t comp_index = sparse[e.Index()];
    REQUIRE(comp_index < dense.size());
    REQUIRE(dense[comp_index].Index() == e.Index());
    REQUIRE(dense.size() == component.size());
}

template <ComponentType T>
static void CheckComponentArrayIntegrity(const ComponentArray<T>& array)
{
    auto check = [&array](const Entity& ent)
    {
        CheckComponentIntegrity(array, ent);
    };

    std::ranges::for_each(CompAccess::Dense(array), check);
}

TEST_CASE("Component Array Constructor", "[comp_array]")
{
    SECTION("Constructor with capacity")
    {
        constexpr size_t capacity = 16;
        const auto comp_array = ComponentArray<Comp1>(capacity);
        REQUIRE(comp_array.Size() == 0);

        /* The sparse array is filled with invalid entity ids. */
        const auto& sparse = CompAccess::Sparse(comp_array);
        REQUIRE(sparse.capacity() == capacity);
        REQUIRE(sparse.size() == capacity);
        std::ranges::for_each(sparse, [](const size_t& val)
        {
            REQUIRE(val == IComponentArray::invalid_entry);
        });

        /* Dense and component arrays are size zero, but memory is reserved. */
        REQUIRE(CompAccess::Dense(comp_array).capacity() == capacity);
        REQUIRE(CompAccess::Components(comp_array).capacity() == capacity);
    }
}

TEST_CASE("ComponentArray Insert() function", "[comp_array]")
{
    SECTION("Random component insertion")
    {
        PRNG rnd(Catch::getSeed());
        auto comp_array = ComponentArray<Comp1>();
        for (uint32_t i = 0; i < 1000; ++i)
        {
            /* Lets choose some random entities in a meaningful boundary, lets say 1 million entities. */
            const auto entity_id = rnd.UniformInt<uint32_t>(1, 1000000);
            const Entity to_be_stored = EntAccess::Create(entity_id, 0);
            comp_array.Insert(to_be_stored, {entity_id});
            CheckComponentIntegrity(comp_array, to_be_stored);

            /* Check the underlying data structures. */
            const size_t sparse_size = CompAccess::Sparse(comp_array).size();
            REQUIRE(sparse_size > entity_id);

            const size_t sparse_index = CompAccess::Sparse(comp_array)[entity_id];
            REQUIRE(CompAccess::Dense(comp_array).size() > sparse_index);

            const Entity stored_entity = CompAccess::Dense(comp_array)[sparse_index];
            REQUIRE(stored_entity == to_be_stored);
            REQUIRE(CompAccess::Components(comp_array)[sparse_index].a == entity_id);
        }

        CheckComponentArrayIntegrity(comp_array);
    }

    SECTION("Triggering sparse array resize on insertion")
    {
        constexpr uint32_t initial_sparse_size = 16;
        auto comp_array = ComponentArray<Comp1>(initial_sparse_size);
        const size_t sparse_size_before_insertion = CompAccess::Sparse(comp_array).size();

        for (uint32_t i = 0; i < initial_sparse_size; ++i)
        {
            const Entity e = EntAccess::Create(i, 0);
            comp_array.Insert(e, {});
            REQUIRE(CompAccess::Sparse(comp_array).size() == sparse_size_before_insertion);
        }
        const Entity e = EntAccess::Create(initial_sparse_size, 0);
        comp_array.Insert(e, {});
        CheckComponentIntegrity(comp_array, e);
        const size_t sparse_size_after_insertion = CompAccess::Sparse(comp_array).size();
        REQUIRE(sparse_size_after_insertion > sparse_size_before_insertion);
        CheckComponentArrayIntegrity(comp_array);
    }

    SECTION("Checking array resize edge cases")
    {
        auto comp_array = ComponentArray<Comp1>(16);
        const size_t sparse_size_before_insertion = CompAccess::Sparse(comp_array).size();
        const Entity e = EntAccess::Create(15, 0);
        comp_array.Insert(e, {});
        CheckComponentIntegrity(comp_array, e);
        const size_t sparse_size_after_insertion = CompAccess::Sparse(comp_array).size();
        REQUIRE(sparse_size_after_insertion == sparse_size_before_insertion);
    }

    SECTION("Checking array resize empty array")
    {
        auto comp_array = ComponentArray<Comp1>(0);
        const size_t sparse_size_before_insertion = CompAccess::Sparse(comp_array).size();
        const Entity e = EntAccess::Create(0, 0);
        comp_array.Insert(e, {});
        CheckComponentIntegrity(comp_array, e);
        const size_t sparse_size_after_insertion = CompAccess::Sparse(comp_array).size();
        REQUIRE(sparse_size_after_insertion > sparse_size_before_insertion);
    }

    SECTION("Checking dense and component size increase on insertion")
    {
        auto comp_array = ComponentArray<Comp1>();
        const size_t dense_size_before_insertion = CompAccess::Dense(comp_array).size();
        const size_t comp_size_before_insertion = CompAccess::Components(comp_array).size();
        const Entity e = EntAccess::Create(0, 0);
        comp_array.Insert(e, {});
        CheckComponentIntegrity(comp_array, e);
        const size_t dense_size_after_insertion = CompAccess::Dense(comp_array).size();
        const size_t comp_size_after_insertion = CompAccess::Components(comp_array).size();
        REQUIRE(dense_size_after_insertion == (dense_size_before_insertion + 1));
        REQUIRE(comp_size_after_insertion == (comp_size_before_insertion + 1));
    }

    SECTION("Checking fast path insertion (component overwritten)")
    {
        auto comp_array = ComponentArray<Comp1>();
        const Entity e = EntAccess::Create(0, 0);
        comp_array.Insert(e, {1});
        CheckComponentIntegrity(comp_array, e);
        const size_t dense_size = CompAccess::Dense(comp_array).size();
        const size_t comp_size = CompAccess::Components(comp_array).size();

        bool ret = comp_array.Insert(e, {2});
        CheckComponentIntegrity(comp_array, e);

        REQUIRE(ret == false);
        REQUIRE(CompAccess::Dense(comp_array).size() == dense_size);
        REQUIRE(CompAccess::Components(comp_array).size() == comp_size);

        const size_t sparse_index = CompAccess::Sparse(comp_array)[e.Index()];
        REQUIRE(CompAccess::Dense(comp_array).size() > sparse_index);
        REQUIRE(CompAccess::Components(comp_array)[sparse_index].a == 2);
    }

    SECTION("Checking new component insertion")
    {
        auto comp_array = ComponentArray<Comp1>();
        const Entity e = EntAccess::Create(0, 0);
        const size_t dense_size_before_insertion = CompAccess::Dense(comp_array).size();
        const size_t comp_size_before_insertion = CompAccess::Components(comp_array).size();
        bool ret = comp_array.Insert(e, {2});
        CheckComponentIntegrity(comp_array, e);
        const size_t dense_size_after_insertion = CompAccess::Dense(comp_array).size();
        const size_t comp_size_after_insertion = CompAccess::Components(comp_array).size();

        REQUIRE(ret == true);
        REQUIRE(dense_size_after_insertion == (dense_size_before_insertion + 1));
        REQUIRE(comp_size_after_insertion == (comp_size_before_insertion + 1));

        const size_t sparse_index = CompAccess::Sparse(comp_array)[e.Index()];
        REQUIRE(CompAccess::Dense(comp_array).size() > sparse_index);
        REQUIRE(CompAccess::Components(comp_array)[sparse_index].a == 2);
    }
}

TEST_CASE("ComponentArray Has() function", "[comp_array]")
{
    SECTION("Checking on empty array")
    {
        const auto comp_array = ComponentArray<Comp1>();
        const Entity e = EntAccess::Create(0, 0);
        REQUIRE(comp_array.Has(e) == false);
    }

    SECTION("Checking on invalid entry")
    {
        const auto comp_array = ComponentArray<Comp1>(1);
        const Entity e = EntAccess::Create(0, 0);
        REQUIRE(comp_array.Has(e) == false);
    }

    SECTION("Checking normal operation")
    {
        auto comp_array = ComponentArray<Comp1>(1);
        const Entity e = EntAccess::Create(0, 0);
        comp_array.Insert(e, {1});
        REQUIRE(comp_array.Has(e) == true);
    }

    SECTION("Checking insertion and removal")
    {
        auto comp_array = ComponentArray<Comp1>(1);
        const Entity e = EntAccess::Create(0, 0);
        comp_array.Insert(e, {1});
        REQUIRE(comp_array.Has(e) == true);
        comp_array.Remove(e);
        REQUIRE(comp_array.Has(e) == false);
    }

    SECTION("Checking generation independence")
    {
        auto comp_array = ComponentArray<Comp1>(1);
        Entity e = EntAccess::Create(0, 0);
        comp_array.Insert(e, {1});
        REQUIRE(comp_array.Has(e) == true);
        EntAccess::Gen(e) = 1;
        comp_array.Insert(e, {1});
        REQUIRE(comp_array.Has(e) == true);
    }

    SECTION("Adding and removing random components")
    {
        PRNG rnd(Catch::getSeed());
        constexpr uint32_t num_tests = 1000;

        std::set<uint32_t> entity_ids;

        auto comp_array = ComponentArray<Comp1>();

        while (entity_ids.size() != num_tests)
        {
            entity_ids.insert(rnd.UniformInt<uint32_t>(0, 1000000));
        }

        std::ranges::for_each(entity_ids, [&comp_array](uint32_t id)
        {
            const Entity e = EntAccess::Create(id, 0);
            comp_array.Insert(e, {0});
        });

        /* Lets remove the component for the first half of the entities. */
        std::set removals(entity_ids.begin(), std::next(entity_ids.begin(), entity_ids.size() / 2));

        std::ranges::for_each(removals, [&comp_array](uint32_t id)
        {
            const Entity e = EntAccess::Create(id, 0);
            comp_array.Remove(e);
        });

        /* Check the status for every entity */
        std::ranges::for_each(entity_ids, [&comp_array, &removals](uint32_t id)
        {
            const Entity e = EntAccess::Create(id, 0);
            const bool has = comp_array.Has(e);

            REQUIRE(has == !removals.contains(id));
        });

        CheckComponentArrayIntegrity(comp_array);
    }
}

TEST_CASE("ComponentArray Remove() function", "[comp_array]")
{
    SECTION("Normal operation")
    {
        auto comp_array = ComponentArray<Comp1>();
        const Entity e = EntAccess::Create(0, 0);
        comp_array.Insert(e, {1});
        REQUIRE(comp_array.Has(e) == true);
        comp_array.Remove(e);
        REQUIRE(comp_array.Has(e) == false);
        REQUIRE(CompAccess::Dense(comp_array).empty());
        REQUIRE(CompAccess::Components(comp_array).empty());
        REQUIRE(CompAccess::Sparse(comp_array)[e.Index()] == IComponentArray::invalid_entry);
    }

    SECTION("Normal operation - swap remove")
    {
        auto comp_array = ComponentArray<Comp1>();
        const Entity e1 = EntAccess::Create(0, 0);
        const Entity e2 = EntAccess::Create(1, 0);
        comp_array.Insert(e1, {1});
        comp_array.Insert(e2, {1});
        REQUIRE(comp_array.Has(e1) == true);
        REQUIRE(comp_array.Has(e2) == true);

        size_t dense_location_e1 = CompAccess::Sparse(comp_array)[e1.Index()];
        comp_array.Remove(e1);

        REQUIRE(CompAccess::Sparse(comp_array)[e2.Index()] == dense_location_e1);
    }
}

TEST_CASE("ComponentArray QueueInsertion() function", "[comp_array]")
{
    SECTION("Normal operation")
    {
        auto comp_array = ComponentArray<Comp1>();
        REQUIRE(CompAccess::PendingOperations(comp_array).empty());
        const Entity e1 = EntAccess::Create(0, 0);
        comp_array.QueueInsertion(e1, {0});
        REQUIRE(CompAccess::PendingOperations(comp_array).size() == 1);
    }
}

TEST_CASE("ComponentArray QueueRemoval() function", "[comp_array]")
{
    SECTION("Normal operation")
    {
        auto comp_array = ComponentArray<Comp1>();
        REQUIRE(CompAccess::PendingOperations(comp_array).empty());
        const Entity e1 = EntAccess::Create(0, 0);
        comp_array.QueueRemoval(e1);
        REQUIRE(CompAccess::PendingOperations(comp_array).size() == 1);
    }
}
