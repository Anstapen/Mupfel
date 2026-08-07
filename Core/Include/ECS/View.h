#pragma once
#include "Components/ComponentIndex.h"
#include "Entity.h"
#include "Registry.h"

#include <cstddef>
#include <iterator>
#include <ranges>
#include <tuple>

namespace Mupfel
{

/**
 * This class can be used to iterate over entities that share a specific set of
 * components. In the current implementation, the view internally iterates over
 * entities which have the FirstComponent and check if the other components are
 * present for that entity.
 */
template <typename FirstComponent, typename... Components>
class View : public std::ranges::view_interface<View<FirstComponent, Components...>>
{
public:
	/**
	 * The first requested component type. The view always iterates over all components.
	 * of this type.
	 */
	using BaseComponent = FirstComponent;

	/**
	 * The Iterator class used for a range based iteration.
	 */
	struct Iterator
	{
		/** The type of tuple that the user will get. */
		using value_type = std::tuple<Entity, FirstComponent&, Components&...>;
		using difference_type = std::ptrdiff_t;
        using iterator_concept = std::forward_iterator_tag;

		Iterator() = default;

		/** Constructor for the iterator. */
		Iterator(const View& in_view, size_t start) : view(&in_view), index(start) { SkipInvalid(); }

		/** Dereference operator. Returns a tuple of the entity and references to its components. */
		value_type operator*() const
		{
			const Entity e{view->base_component_array->dense[index]};

			return value_type(
				e, view->base_component_array->components[index],
				std::get<ComponentArray<Components>*>(view->component_arrays)->Get(e)...);
		}

		/** Pre-increment operator. Returns the incremented iterator. */
		Iterator& operator++()
		{
			++index;
			SkipInvalid();
			return *this;
		}

		/** Post increment operator. Returns the not-incremented iterator. */
		Iterator operator++(int)
		{
			Iterator previous = *this;
			++*this;
			return previous;
		}

		/**
		 * The comparison with the sentinel ("end iterator").
		 * This comparison is true (i.e. the exit condition for the iteration is hit)
		 * when the index hits the end of the base component array.
		 */
		bool operator==(std::default_sentinel_t) const { return index >= view->base_component_array->Size(); }

		/**
		 * Comparison with another Iterator.
		 */
		bool operator==(const Iterator& other) const { return index == other.index; }

	private:
		/** Advances index until it points at an entity satisfying required_signature, or past the end. */
		void SkipInvalid()
		{
			while (index < view->base_component_array->Size() &&
				   !view->Matches(Entity{view->base_component_array->dense[index]}))
			{
				++index;
			}
		}

		/** Pointer to the view that created the iterator. */
		const View* view = nullptr;
		/** Current slot in the iteration. */
		size_t index = 0;
	};

public:
	/** Constructuor of a View. It takes a reference to the registry and initializes all
	 * other members using it.
	 */
	explicit View(Registry& in_reg)
		: reg(&in_reg), required_signature(Registry::ComponentSignature<FirstComponent, Components...>()),
		  required_scene(in_reg.GetActiveSceneMask()),
		  base_component_array(&in_reg.GetComponentArray<FirstComponent>()),
		  component_arrays(&in_reg.GetComponentArray<Components>()...)

	{
	}

	/** The starting iterator. */
	Iterator begin() const { return Iterator(*this, 0); }

	/**
	 * Return a default sentinel. The end condition will be checked dynamically by the iterator.
	 */
	std::default_sentinel_t end() const { return std::default_sentinel; }

private:
	/**
	 * Predicate stating whether or not the given entity has the wanted components AND
	 * is part of the current scene.
	 * 
	 * \param e The entity to check.
	 * \return True if the entity is part of the current scene AND has the wanted components, FALSE otherwise.
	 */
	bool Matches(Entity e) const;
private:
	/** The registry this view iterates. */
	Registry* reg = nullptr;
	/** Combined signature mask. */
	Entity::Signature required_signature{};
	/**  Scene mask. */
	Scene::SceneMask						   required_scene{};
	/**
	 * The component array of the first provided component type.
	 * Currently, this component array is iterated and the contained entities
	 * are checked for the other components.
	 */
	ComponentArray<FirstComponent>*			   base_component_array = nullptr;

	/** Component arrays of the other component types. */
	std::tuple<ComponentArray<Components>*...> component_arrays{};
};

template <typename FirstComponent, typename... Components>
inline bool View<FirstComponent, Components...>::Matches(Entity e) const
{
	/* retrieve scene and signature from the entity. */
	const Entity::Signature sig = reg->GetSignature(e);
	const Scene::SceneMask	scene = reg->GetSceneMask(e);

	return ((sig & required_signature) == required_signature) && ((scene & required_scene) == required_scene);
}

} // namespace Mupfel