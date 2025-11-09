#pragma once
#include "Registry.h"
#include "Entity.h"
#include <tuple>
#include <functional>

namespace Mupfel {

    template<typename... Components>
    class View {
    public:
        explicit View(Registry& in_reg)
            : reg(in_reg),
            component_arrays(std::make_tuple(&reg.GetComponentArray<Components>()...)) {
        }

        struct Iterator {
            Registry& registry;
            const uint32_t *entities;
            const uint32_t num_entities;
            size_t index = 0;
            const Entity::Signature required;
            std::tuple<ComponentArray<Components>*...> arrays;

            Iterator(Registry& in_reg,
                const uint32_t* ents,
                const uint32_t num_ents,
                Entity::Signature req,
                std::tuple<ComponentArray<Components>*...> arrs,
                size_t idx)
                : registry(in_reg), entities(ents), index(idx), required(req), arrays(arrs), num_entities(num_ents)
            {
                SkipInvalid();
            }

            void SkipInvalid() {
                while (index < num_entities) {
                    uint32_t eIndex = entities[index];
                    const auto& sig = registry.GetSignature(eIndex);
                    if ((sig & required) == required)
                        break;
                    ++index;
                }
            }

            bool operator!=(const Iterator& other) const { return index != other.index; }

            void operator++() {
                ++index;
                SkipInvalid();
            }

            // liefert tuple: (Entity, Component&...)
            auto operator*() {
                Entity e{ entities[index] };
                auto tuple_of_refs = std::apply(
                    [&](auto*... arr) {
                        return std::tie(arr->Get(e)...); // echte Referenzen
                    },
                    arrays
                );
                return std::tuple_cat(std::make_tuple(e), tuple_of_refs);
            }
        };

    public:
        Iterator begin() {
            using BaseComponent = std::tuple_element_t<0, std::tuple<Components...>>;
            auto& array = reg.GetComponentArray<BaseComponent>();
            return Iterator(reg, array.GetDense(), array.GetDenseSize(), Registry::ComponentSignature(), component_arrays, 0);
        }

        Iterator end() {
            using BaseComponent = std::tuple_element_t<0, std::tuple<Components...>>;
            auto& array = reg.GetComponentArray<BaseComponent>();
            return Iterator(reg, array.GetDense(), array.GetDenseSize(), Registry::ComponentSignature(), component_arrays, array.GetDenseSize());
        }

    private:
        Registry& reg;
        std::tuple<ComponentArray<Components>*...> component_arrays;
    };

}