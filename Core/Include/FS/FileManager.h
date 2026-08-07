#pragma once
#include <filesystem>
#include <cstdint>

namespace Mupfel
{

	class FileManager
	{
	public:
		class Handle {
			uint64_t id;
			std::filesystem::path file;
		};
	public:
		virtual Handle Load(std::filesystem::path file) = 0;
		virtual void Store(const Handle &handle) = 0;
	};

}