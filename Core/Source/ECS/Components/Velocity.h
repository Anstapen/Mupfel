#pragma once

namespace Mupfel {
	struct alignas(16) Velocity {
		float x, y, z = 0.0f;
		float angular = 0.0f;
	};
}
