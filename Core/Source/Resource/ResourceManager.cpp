#include "../../Include/Core/ResourceManager.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using namespace Mupfel;

struct MemoryResource
{
	std::array<char, ResourceManager::MAX_FILE_NAME_LENGTH> name{'\0'};
	uint64_t												offset = 0;
	uint64_t												size = 0;
};

ResourceManager::~ResourceManager() { raw_file.close(); }

bool ResourceManager::SaveFile(const std::string& file)
{
	if (!std::filesystem::exists(file))
	{
		return false;
	}

	resourceMap.try_emplace(file, ResourceManager::Resource());

	return true;
}

std::shared_ptr<std::vector<uint8_t>> ResourceManager::GetFile(const std::string& file)
{
	auto it = resourceMap.find(file);
	if (it == resourceMap.end())
	{
		return {};
	}

	if (it->second.size == 0)
	{
		return {};
	}

	raw_file.clear();
	raw_file.seekg(static_cast<std::streamoff>(it->second.offset));

	auto file_mem = std::make_shared<std::vector<uint8_t>>(it->second.size);
	raw_file.read(reinterpret_cast<char*>(file_mem->data()),
				  static_cast<std::streamsize>(it->second.size));

	if (raw_file.gcount() != static_cast<std::streamsize>(it->second.size))
	{
		return {};
	}
	return file_mem;
}

bool ResourceManager::Load(const std::string& file, std::string_view key)
{
	(void)key;
	raw_file.open(file, std::ios_base::binary);

	if (!raw_file.is_open())
	{
		return false;
	}

	uint32_t num_files = 0;
	raw_file.read(reinterpret_cast<char*>(&num_files), sizeof(uint32_t));

	for (uint32_t i = 0; i < num_files; i++)
	{
		MemoryResource m;
		raw_file.read(reinterpret_cast<char*>(&m), sizeof(MemoryResource));
		std::string s(
			m.name.begin(),
			m.name.begin() + std::min<size_t>(strlen(m.name.data()), ResourceManager::MAX_FILE_NAME_LENGTH));
		resourceMap[s] = {.offset = m.offset, .size = m.size};
	}

	return true;
}

bool ResourceManager::Store(const std::string& file, std::string_view key)
{
	(void)key;
	if (resourceMap.empty())
	{
		return false;
	}

	std::ofstream out;
	out.open(file, std::ios_base::binary);

	if (!out.is_open())
	{
		return false;
	}

	/* Write the number of map entries. */
	auto num_files = static_cast<uint32_t>(resourceMap.size());
	out.write(reinterpret_cast<char*>(&num_files), sizeof(uint32_t));

	/* Dummy writes of the offsets, to create the complete header. */
	for (auto& [k, v] : resourceMap)
	{
		MemoryResource m;
		std::memcpy(
			m.name.data(), k.data(),
			std::min<uint32_t>(static_cast<uint32_t>(k.size()), ResourceManager::MAX_FILE_NAME_LENGTH));
		m.offset = v.offset;
		m.size = v.size;

		out.write(reinterpret_cast<char*>(&m), sizeof(MemoryResource));
	}

	/* Write the files */
	for (auto& [k, v] : resourceMap)
	{
		std::ifstream tmp_file(k, std::ios_base::binary | std::ios_base::ate);

		if (!tmp_file.is_open())
		{
			std::cout << "Unable to open file " << k << " for reading, skipping..." << std::endl;
			continue;
		}

		const std::streamsize file_size = tmp_file.tellg();
		tmp_file.seekg(0);

		std::vector<uint8_t> file_mem(static_cast<size_t>(file_size));
		tmp_file.read(reinterpret_cast<char*>(file_mem.data()), file_size);

		v.offset = out.tellp();
		v.size = file_mem.size();

		out.write(reinterpret_cast<const char*>(file_mem.data()), file_size);
	}

	/* Write the map again, now with updated offsets. */
	out.seekp(4);
	for (auto& [k, v] : resourceMap)
	{
		MemoryResource m;
		std::memcpy(
			m.name.data(), k.data(),
			std::min<uint32_t>(static_cast<uint32_t>(k.size()), ResourceManager::MAX_FILE_NAME_LENGTH));
		m.offset = v.offset;
		m.size = v.size;

		out.write(reinterpret_cast<char*>(&m), sizeof(MemoryResource));
	}

	out.close();

	return true;
}

void ResourceManager::PrintFiles()
{
	for (const auto& [key, value] : resourceMap)
	{
		std::cout << key << std::endl;
	}
}

void ResourceManager::scramble(std::vector<uint8_t>& stream, uint64_t key)
{
	const unsigned char* enc = reinterpret_cast<unsigned char*>(&key);

	for (uint32_t i = 0; i < stream.size(); i++)
	{
		stream[i] ^= enc[i % 8];
	}
}
