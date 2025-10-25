#pragma once
#include "MovementSystem.h"
#include "Core/EventSystem.h"
#include "ECS/Registry.h"
#include <cstdint>
#include <vector>

namespace Mupfel {

	class MovementSystemGPU :
		public MovementSystem
	{
	public:
		MovementSystemGPU(Registry& in_reg, uint32_t in_max_entities = 1000);
		virtual ~MovementSystemGPU();
		void Init() override;
		void DeInit() override;
		void Update(double elapsedTime) override;
		
	private:
		void UpdateEntities();
		void AddedComponentHandler(const ComponentAddedEvent& evt);
		void RemovedComponentHandler(const ComponentRemovedEvent& evt);
		void AddEntity(Entity e);
		void RemoveEntity(Entity e);
		void CreateOrResizeSSBO(uint32_t capacity);
		static constexpr size_t invalid_entry = std::numeric_limits<size_t>::max();
		std::vector<size_t> sparse;
		std::vector<uint32_t> dense;
		unsigned int shader_id;
		unsigned int ssbo_id;
		void* mapped_ssbo;
		uint32_t current_ssbo_index;
	};

}


