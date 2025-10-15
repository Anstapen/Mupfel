#include "Circle.h"
#include "raylib.h"

void Mupfel::Circle::RayLibDrawCircleLines(int centerX, int centerY, float radius)
{
	DrawCircleLines(centerX, centerY, radius, PINK);
}

void Mupfel::Circle::RayLibDrawCircleLines(int centerX, int centerY, float radius, int r, int g, int b, int a)
{
	Color c = { r, g, b, a };
	DrawCircleLines(centerX, centerY, radius, c);
}
