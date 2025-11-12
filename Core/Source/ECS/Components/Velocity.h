#pragma once

namespace Mupfel {
	struct alignas(16) Velocity {
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float angular = 0.0f;
	};
}
