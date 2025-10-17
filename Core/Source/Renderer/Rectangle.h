#pragma once
#include <cstdint>

namespace Mupfel {

	class Rectangle
	{
	public:
		static void RaylibDrawRect(int posX, int posY, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
		static void RaylibDrawRectFilled(int posX, int posY, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
	};

}



