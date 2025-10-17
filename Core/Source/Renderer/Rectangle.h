#pragma once

namespace Mupfel {

	class Rectangle
	{
	public:
		static void RaylibDrawRect(int posX, int posY, int width, int height, int r, int g, int b, int alpha);
		static void RaylibDrawRectFilled(int posX, int posY, int width, int height, int r, int g, int b, int alpha);
	};

}



