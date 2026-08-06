#pragma once
#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <streambuf>
#include <string_view>
#include <map>
#include <vector>

namespace Mupfel
{
class ResourceManager : public std::streambuf
{
	using SafeBufferPtr = std::shared_ptr<std::vector<uint8_t>>;
	static constexpr uint32_t MAX_FILE_NAME_LENGTH = 256;

public:
	ResourceManager();
	~ResourceManager();
	bool		  Load(std::string_view file, std::string_view key);
	bool		  Save(std::string_view file, std::string_view key);
	SafeBufferPtr GetFile(std::string_view file);
	bool		  AddFile(const std::string& file);

private:
	struct Resource
	{
		uint32_t							   offset = 0;
		uint32_t							   size = 0;
	};

	std::map<std::string, Resource> fileMap;
	std::ifstream							  resourceFile;
};

} // namespace Mupfel
