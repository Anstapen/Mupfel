#include "Text.h"
#include "raylib.h"

void Mupfel::Text::RaylibDrawText(const char* text, int posX, int posY, int fontSize)
{
	DrawText(text, posX, posY, fontSize, GREEN);
}
