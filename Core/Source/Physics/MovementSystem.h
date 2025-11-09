#pragma once
#include <cstdint>
#include <vector>

/* For the Component Event definitions */
#include "ECS/Registry.h"

namespace Mupfel {

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
		void JoinAndMovement(double elapsedTime);

	private:
		uint32_t movement_update_shader_id;
		uint32_t join_shader_id;
	};

}


