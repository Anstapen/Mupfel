#pragma once
#include <string_view>
#include "Texture.h"
#include <memory>

namespace Mupfel {

	using SafeTexturePointer = std::shared_ptr< Mupfel::Texture>;
	using TextureHandle = uint64_t;


	class TextureManager
	{
	public:
		static SafeTexturePointer LoadTextureFromFile(std::string_view path);
	private:
		TextureManager() {};
	};

}



