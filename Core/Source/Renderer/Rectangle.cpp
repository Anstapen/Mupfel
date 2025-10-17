#include "Rectangle.h"
#include "raylib.h"
#include <cstdint>

void Mupfel::Rectangle::RaylibDrawRect(int posX, int posY, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
	Color c = { r, g, b, alpha };
	DrawRectangleLines(posX, posY, width, height, c);
}

void Mupfel::Rectangle::RaylibDrawRectFilled(int posX, int posY, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
	Color c = { r, g, b, alpha };
	DrawRectangle(posX, posY, width, height, c);
}
