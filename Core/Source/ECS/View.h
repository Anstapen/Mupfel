#pragma once
#include "Registry.h"
#include "Entity.h"
#include <tuple>
#include <functional>
#include "Core/GUID.h"

namespace Mupfel {

	class Registry;

	template<typename... Components>
	class View {
	public:
		explicit View(Registry& in_reg) : reg(in_reg),
			component_arrays(std::make_tuple(&reg.GetComponentArray<Components>()...)){}

		struct Iterator {
			Registry& registry;
			const std::vector<uint32_t>& entities;
			size_t index = 0;
			const Entity::Signature required;
			std::tuple<ComponentArray<Components>*...> arrays;

			Iterator(Registry& in_reg, const std::vector<uint32_t>& ents, Entity::Signature req, std::tuple<ComponentArray<Components>*...> arrs, size_t idx) :
				registry(in_reg), entities(ents), index(idx), required(req), arrays(arrs)
			{
				SkipInvalid();
			}

			void SkipInvalid() {
				while (index < entities.size())
				{
					uint32_t eIndex = entities[index];

					const auto& sig = registry.GetSignature(eIndex);
					if ((sig & required) == required)
					{
						break;
					}

					index++;
				}
			}

			bool operator!=(const Iterator& other) const { return index != other.index; }

			void operator++() {
				++index;
				SkipInvalid();
			}

			auto operator*() {
				Entity e{ entities[index]};
				// wende Get(e) für jedes Array im Tuple an
				auto tuple_of_refs = std::apply(
					[&](auto*... arr) {
						return std::make_tuple(std::ref(arr->Get(e))...);
					},
					arrays
				);
				return std::tuple_cat(std::make_tuple(e), tuple_of_refs);
				//return std::make_tuple(e, std::ref((registry.GetComponent<Components>(e)))...);
			}
		};

	public:
		Iterator begin() {
			using BaseComponent = std::tuple_element_t<0, std::tuple<Components...>>;
			auto& array = reg.GetComponentArray<BaseComponent>();
			return Iterator(reg, array.dense, Registry::ComponentSignature(), component_arrays, 0);
		}

		Iterator end() {
			using BaseComponent = std::tuple_element_t<0, std::tuple<Components...>>;
			auto& array = reg.GetComponentArray<BaseComponent>();
			return Iterator(reg, array.dense, Registry::ComponentSignature(), component_arrays, array.dense.size());
		}

	private:
		Registry& reg;
		std::tuple<ComponentArray<Components>*...> component_arrays;
	};

}
