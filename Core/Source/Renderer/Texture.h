#pragma once
#include <cstdint>
#include <string_view>
#include <memory>


namespace Mupfel {

	struct Texture {
		unsigned int id = 0;        // OpenGL texture id
		int width = 0;              // Texture base width
		int height = 0;             // Texture base height
		int mipmaps = 0;            // Mipmap levels, 1 by default
		int format = 0;				// Data format (PixelFormat type)
		static void RaylibDrawTexture(const Texture& t, int posX, int posY);
		virtual ~Texture();
		Texture(std::string_view path);
	};

	typedef std::shared_ptr< Texture> SafeTexturePointer;

	struct TextureComponent {
		TextureComponent(uint64_t in_id) : id(in_id) {}
		TextureComponent() : id(0) {}
		uint64_t id;
	};
}

