#include "Rectangle.h"
#include "raylib.h"

void Mupfel::Rectangle::RaylibDrawRect(int posX, int posY, int width, int height, int r, int g, int b, int alpha)
{
	Color c = { r, g, b, alpha };
	DrawRectangleLines(posX, posY, width, height, c);
}

void Mupfel::Rectangle::RaylibDrawRectFilled(int posX, int posY, int width, int height, int r, int g, int b, int alpha)
{
	Color c = { r, g, b, alpha };
	DrawRectangle(posX, posY, width, height, c);
}
