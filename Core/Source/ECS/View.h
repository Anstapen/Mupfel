#pragma once
#include "Registry.h"
#include "Entity.h"

#include <tuple>
#include <functional>

namespace Mupfel {

	class Registry;

	template<typename... Components>
	class View {
	public:
		explicit View(Registry& in_reg) : reg(in_reg) {}

		struct Iterator {
			Registry& registry;
			const std::vector<uint32_t>& entities;
			size_t index = 0;
			const Entity::Signature required;

			Iterator(Registry& in_reg, const std::vector<uint32_t>& ents, Entity::Signature req, size_t idx) :
				registry(in_reg), entities(ents), index(idx), required(req)
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
				return std::make_tuple(e, std::ref((registry.GetComponent<Components>(e)))...);
			}
		};

	public:
		Iterator begin() {
			using BaseComponent = std::tuple_element_t<0, std::tuple<Components...>>;
			auto& array = reg.GetComponentArray<BaseComponent>();
			return Iterator(reg, array.dense, requiredSignature(), 0);
		}

		Iterator end() {
			using BaseComponent = std::tuple_element_t<0, std::tuple<Components...>>;
			auto& array = reg.GetComponentArray<BaseComponent>();
			return Iterator(reg, array.dense, requiredSignature(), array.dense.size());
		}

	private:
		static Entity::Signature requiredSignature();
	private:
		Registry& reg;
	};

	template<typename ...Components>
	inline Entity::Signature View<Components...>::requiredSignature()
	{
		Entity::Signature sig;
		(sig.set(CompUtil::GetComponentTypeID<Components>()), ...);
		return sig;
	}

}
