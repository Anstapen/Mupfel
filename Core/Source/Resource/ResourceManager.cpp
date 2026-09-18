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

	std::string key = file;
	resourceMap.try_emplace(key, ResourceManager::Resource());

	return true;
}

std::shared_ptr<std::vector<uint8_t>> ResourceManager::GetFile(const std::string& file)
{
	auto it = resourceMap.find(file);
	if (it == resourceMap.end())
	{
		return std::shared_ptr<std::vector<uint8_t>>();
	}

	if (it->second.size == 0)
	{
		return std::shared_ptr<std::vector<uint8_t>>();
	}

	raw_file.seekg(it->second.offset);

	uint64_t end = it->second.offset + it->second.size;

	std::vector<uint8_t>* file_mem = new std::vector<uint8_t>();
	int					  c;

	while (!raw_file.eof())
	{
		c = raw_file.get();
		if (raw_file.eof() || (raw_file.tellg() > static_cast<int64_t>(end)))
		{
			break;
		}
		file_mem->push_back(static_cast<uint8_t>(c));
	}

	return std::shared_ptr<std::vector<uint8_t>>(file_mem);
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
	raw_file.read((char*)&num_files, sizeof(uint32_t));

	for (uint32_t i = 0; i < num_files; i++)
	{
		MemoryResource m;
		raw_file.read((char*)&m, sizeof(MemoryResource));
		std::string s(
			m.name.begin(),
			m.name.begin() + std::min<size_t>(strlen(m.name.data()), ResourceManager::MAX_FILE_NAME_LENGTH));
		resourceMap[s] = {m.offset, m.size};
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
	uint32_t num_files = static_cast<uint32_t>(resourceMap.size());
	out.write((char*)&num_files, sizeof(uint32_t));

	/* Dummy writes of the offsets, to create the complete header. */
	for (auto& [k, v] : resourceMap)
	{
		MemoryResource m;
		std::memcpy(
			m.name.data(), k.data(),
			std::min<uint32_t>(static_cast<uint32_t>(k.size()), ResourceManager::MAX_FILE_NAME_LENGTH));
		m.offset = v.offset;
		m.size = v.size;

		out.write((char*)&m, sizeof(MemoryResource));
	}

	/* Write the files */
	for (auto& [k, v] : resourceMap)
	{
		std::ifstream tmp_file;
		tmp_file.open(k, std::ios_base::binary);

		if (!tmp_file.is_open())
		{
			std::cout << "Unable to open file " << k << " for reading, skipping..." << std::endl;
			continue;
		}

		/* For now, we copy it into memory */
		std::vector<uint8_t> file_mem;
		int					 c;

		while (!tmp_file.eof())
		{
			c = tmp_file.get();
			if (tmp_file.eof())
			{
				break;
			}
			file_mem.push_back(static_cast<uint8_t>(c));
		}

		v.offset = out.tellp();
		v.size = file_mem.size();

		out.write((char*)file_mem.data(), file_mem.size());
		tmp_file.close();
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

		out.write((char*)&m, sizeof(MemoryResource));
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

	for (int i = 0; i < stream.size(); i++)
	{
		stream[i] ^= enc[i % 8];
	}
}
