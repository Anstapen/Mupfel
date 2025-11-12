#pragma once

namespace Mupfel {

	struct alignas(16) Transform
	{
		float pos_x = 0.0f;
		float pos_y = 0.0f;
		float pos_z = 0.0f;
		float _pad0 = 0.0f;
		float scale_x = 1.0f;
		float scale_y = 1.0f;
		float rotation = 0.0f;
		float _padding[1];
	};

}



