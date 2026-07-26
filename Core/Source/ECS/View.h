#pragma once
#include "Components/ComponentIndex.h"
#include "Entity.h"
#include "Registry.h"
#include <functional>
#include <tuple>

namespace Mupfel
{

	/**
	 * Iterable range over every entity in `reg` that has all of `Components...`, yielding
	 * `std::tuple<Entity, Components&...>` (usable with structured bindings, e.g.
	 * `for (auto [e, transform, texture] : registry.view<Transform, Texture>())`).
	 *
	 * Iterates the dense array of the first component type in `Components...` and filters by
	 * signature, so it's cheapest when that first type is the rarest of the requested components.
	 */
	template <typename FirstComponent, typename... Components> class View
	{
	public:
		/** The first requested component type; its dense array drives iteration. */
		using BaseComponent = FirstComponent;

		/**
		 * Computes the combined signature for `Components...` up front so each iterator step is a cheap
		 * bitmask compare.
		 */
		explicit View(Registry& in_reg) : reg(in_reg)
		{
			// Erzeuge die benoetigte Components-Signatur
			required_signature = (1ull << ComponentIndex::Index<FirstComponent>()) |
				(0ull | ... | (1ull << ComponentIndex::Index<Components>()));
		}

		/** Forward iterator over `BaseComponent`'s dense array, skipping entities missing any of `Components...`. */
		struct Iterator
		{
			/** The registry being iterated. */
			Registry& registry;
			/** Current slot in `BaseComponent`'s dense array. */
			size_t index;
			/** Combined signature every yielded entity must satisfy. */
			Entity::Signature required_signature;
			/** Cached component array of first component. */
			ComponentArray<BaseComponent>& base_component_array;
			std::tuple<ComponentArray<Components>*...> component_arrays;

			/** Starts at slot `idx` and immediately skips forward past any entity missing a required component. */
			Iterator(Registry& reg, uint64_t req, size_t idx, ComponentArray<BaseComponent>& in_base_component_array, std::tuple<ComponentArray<Components>*...> in_component_arrays)
				: registry(reg), index(idx), required_signature(req), base_component_array(in_base_component_array), component_arrays(in_component_arrays)
			{
					SkipInvalid();
			}

			/** Advances `index` until it points at an entity satisfying `required_signature`, or past the end. */
			void SkipInvalid()
			{
				while (index < base_component_array.Size())
				{
					Entity			  e{ base_component_array.dense[index] };
					Entity::Signature sig = registry.GetSignature(e.Index());

					if ((sig & required_signature) == required_signature)
						break;

					++index;
				}
			}

			/** Whether `o` is at a different slot (used for the `begin() != end()` loop condition). */
			bool operator!=(const Iterator& o) const { return index != o.index; }

			/** Advances to the next matching entity. */
			void operator++()
			{
				++index;
				SkipInvalid();
			}

			/** @return `{entity, Components&...}` for the entity at the current slot. */
			auto operator*()
			{
				Entity e{ base_component_array.dense[index] };

				return std::apply(
					[&](auto*... arrays)
					{
						return std::tuple<Entity, FirstComponent&, Components&...>(
							e,
							base_component_array.components[index],
							arrays->Get(e)...);
					},
					component_arrays);
			}
		};

	public:
		/** Iterator at the first matching entity (or `end()` if there are none). */
		Iterator begin() { return Iterator(reg, required_signature, 0, reg.GetComponentArray<BaseComponent>(), std::tuple<ComponentArray<Components>*...>(&reg.GetComponentArray<Components>()...)); }

		/**
		 * @note Recomputes `BaseComponent`'s current size — only stable as long as the array isn't
		 * mutated during iteration.
		 */
		Iterator end()
		{
			auto& array = reg.GetComponentArray<BaseComponent>();
			return Iterator(reg, required_signature, array.Size(), reg.GetComponentArray<BaseComponent>(), std::tuple<ComponentArray<Components>*...>(&reg.GetComponentArray<Components>()...));
		}

	private:
		/** The registry this view iterates. */
		Registry& reg;
		/** Combined signature bit for each of `Components...`. */
		uint64_t required_signature = 0;
	};

} // namespace Mupfel