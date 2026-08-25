#pragma once

namespace Mupfel {

    /**
     * @brief Component that stores per-entity movement parameters.
     *
     * The Movement component is used by the Movement System to update
     * an entity's Transform each frame. It defines the linear and angular
     * velocity of an entity, as well as optional acceleration and friction
     * parameters that allow smooth, natural motion.
     *
     * This component is tightly aligned to 16 bytes, as it is stored as a
     * GPU buffer.
     */
    struct Movement {
        float velocity_x = 0.0f;
        float velocity_y = 0.0f;
        float velocity_z = 0.0f;
		float angular_velocity = 0.0f;
    };
}
