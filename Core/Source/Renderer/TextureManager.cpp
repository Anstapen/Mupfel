#include "TextureManager.h"
#include <unordered_map>
#include "raylib.h"
#include "Core/GUID.h"
#include "Texture.h"
#include <iostream>

using WeakTexturePointer = std::weak_ptr<Mupfel::Texture>;

static std::unordered_map<uint64_t, WeakTexturePointer> textures;

Mupfel::SafeTexturePointer Mupfel::TextureManager::LoadTextureFromFile(std::string_view path)
{
	/* Check if the image specified by path is already */
	uint64_t hashed_path = Hash::Compute_n(path.data(), path.size());

	auto it = textures.find(hashed_path);

	if (it == textures.end() || it->second.expired())
	{
		std::cout << "Loading Texture from path: " << path << std::endl;
		/* Texture is not loaded, load it from the given File */
		
		if (!FileExists(path.data()))
		{
			std::cout << "ERROR Could not find file: " << path << std::endl;
			return nullptr;
		}

		/* Create a shared pointer to be used */
		auto safe_txtr_ptr = std::make_shared<Mupfel::Texture>(path.data());
		
		textures[hashed_path] = safe_txtr_ptr;


		return safe_txtr_ptr;
	}
	else {
		/* Texture is present, create a new shared_ptr from the weak ptr */
		return it->second.lock();
	}
}
