#pragma once
#include <cstdint>
#include <vector>

/* For the Component Event definitions */
#include "ECS/Registry.h"

namespace Mupfel {

	/**
	 * @brief Simple Event that gets emitted after run of the Movement Compute Shader
	 * to notify other systems.
	 */
	class MovementSystemUpdateEvent : public Event<MovementSystemUpdateEvent> {
	public:
		MovementSystemUpdateEvent() : ssbo_id(0), ssbo_size(0) {};
		MovementSystemUpdateEvent(uint32_t in_ssbo_id, uint32_t in_ssbo_size) : ssbo_id(in_ssbo_id), ssbo_size(in_ssbo_size) {};
		virtual ~MovementSystemUpdateEvent() = default;


		static constexpr uint64_t GetGUIDStatic() {
			return Hash::Compute("MovementSystemUpdateEvent");
		}

	public:
		/**
		 * @brief The current OpenGL SSBO id,
		 *        it may have changed if the buffer got resized.
		 */
		uint32_t ssbo_id;

		/**
		 * @brief The currently used SSBO size = the number of
		 *        entities currently updated by the Movement System.
		 */
		uint32_t ssbo_size;
	};

	/**
	 * @brief This class implements the Movement System of Mupfel. This acts as a Sub-System of the Physics-Pipeline.
	 * It updates specific 
	 */
	class MovementSystem {
	public:
		MovementSystem(uint32_t in_max_entities = 1000);
		virtual ~MovementSystem();
		void Init();
		void DeInit();
		void Update(double elapsedTime);

	private:
		uint32_t shader_id;
	};

}


