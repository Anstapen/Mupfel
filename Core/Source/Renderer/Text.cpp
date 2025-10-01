#include "Text.h"
#include "raylib.h"

void Mupfel::Text::RaylibDrawText(const char* text, int posX, int posY)
{
	ClearBackground(RAYWHITE);
	DrawText(text, posX, posY, 20, LIGHTGRAY);
}
