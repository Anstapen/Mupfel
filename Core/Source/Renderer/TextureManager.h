#pragma once
#include <string_view>
#include "Texture.h"
#include <memory>

namespace Mupfel {

	class TextureManager
	{
	public:
		static SafeTexturePointer LoadTextureFromFile(std::string_view path);
	private:
		TextureManager() {};
	};

}



