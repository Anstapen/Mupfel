#pragma once
#include "Core/Coordinate.h"

namespace Mupfel {

	struct Transform
	{
		Coordinate<float> pos;
		float scale_x = 1.0f;
		float scale_y = 1.0f;
		float rotation = 0.0f;
	};

}



