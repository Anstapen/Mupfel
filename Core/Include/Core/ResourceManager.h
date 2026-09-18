#pragma once
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Mupfel
{

class ResourceManager
{
public:
	static constexpr uint32_t MAX_FILE_NAME_LENGTH = 256;

public:
	~ResourceManager();
	ResourceManager() = default;
	ResourceManager(const ResourceManager& other) = delete;
	ResourceManager(ResourceManager&& other) = delete;
	ResourceManager&					  operator=(const ResourceManager& other) = delete;
	ResourceManager&					  operator=(ResourceManager&& other) = delete;
	bool								  SaveFile(const std::string& file);
	std::shared_ptr<std::vector<uint8_t>> GetFile(const std::string& file);
	bool								  Load(const std::string& file, std::string_view key = "");
	bool								  Store(const std::string& file, std::string_view key = "");

	void PrintFiles();

private:
	void scramble(std::vector<uint8_t>& stream, uint64_t key);

public:
	struct Resource
	{
		uint64_t offset;
		uint64_t size;
	};

private:
	std::map<std::string, Resource> resourceMap;
	std::ifstream					raw_file;
};

} // namespace Mupfel
