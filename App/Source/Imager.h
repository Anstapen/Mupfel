#pragma once
#include "Mupfel.h"
#include <string>
#include <unordered_map>
#include <vector>

class Imager
{
public:
	static bool Load(const std::string& in_handle, const std::string& path, Mupfel::ResourceManager* mngr = nullptr);
	static bool LoadAnimated(
		const std::string&				  in_handle,
		const std::string&				  path,
		const Mupfel::ImageSpecification& spec,
		Mupfel::ResourceManager*		  mngr = nullptr);
	static bool LoadSpriteSheet(
		const std::string&				  in_handle,
		const std::string&				  path,
		const Mupfel::ImageSpecification& spec,
		Mupfel::ResourceManager*		  mngr = nullptr);
	static Mupfel::ImageHandle					   Get(const std::string& in_handle);
	static const std::vector<Mupfel::ImageHandle>& GetSpriteSheet(const std::string& in_handle);

private:
	static std::unordered_map<std::string, Mupfel::ImageHandle>				 image_map;
	static std::unordered_map<std::string, std::vector<Mupfel::ImageHandle>> spritesheet_map;
};
