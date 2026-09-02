#include "ResourceManager.h"
#include <filesystem>
#include <string>

using namespace Mupfel;

Mupfel::ResourceManager::ResourceManager() {}

Mupfel::ResourceManager::~ResourceManager() { resourceFile.close(); }

bool Mupfel::ResourceManager::Load(std::string_view file, std::string_view key)
{
	(void)file;
	(void)key;
	return true;
}

bool Mupfel::ResourceManager::Save(std::string_view file, std::string_view key)
{
	(void)file;
	(void)key;
	return true;
}

ResourceManager::SafeBufferPtr Mupfel::ResourceManager::GetFile(std::string_view file)
{
	(void)file;
	return SafeBufferPtr();
}

bool Mupfel::ResourceManager::AddFile(const std::string& file)
{
	(void)file;
	return false;
}
