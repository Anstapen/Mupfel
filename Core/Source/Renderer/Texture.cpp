#include "Texture.h"
#include "raylib.h"
#include <iostream>

static Texture2D Get2DFromTex(const Mupfel::Texture& t)
{
	return { t.id, t.width, t.height, t.mipmaps, t.format };
}

void Mupfel::Texture::RaylibDrawTexture(const Texture& t, int posX, int posY)
{
	DrawTexture(Get2DFromTex(t), posX, posY, WHITE);
}

Mupfel::Texture::~Texture()
{
	UnloadTexture(Get2DFromTex(*this));
}

Mupfel::Texture::Texture(std::string_view path)
{
	Image img = LoadImage(path.data());

	/* If Image Loading failed, return */
	if (img.data == nullptr)
	{
		std::cout << "WARNING: Construction of Texture FAILED!" << path << std::endl;
		UnloadImage(img);
		return;
	}

	Texture2D t = LoadTextureFromImage(img);
	UnloadImage(img);

	id = t.id;
	width = t.width;
	height = t.height;
	mipmaps = t.mipmaps;
	format = t.format;

}


