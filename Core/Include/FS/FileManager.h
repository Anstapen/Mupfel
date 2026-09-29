#pragma once
#include <filesystem>
#include <cstdint>

namespace Mupfel
{

	class FileManager
	{
	public:
		virtual ~FileManager() = default;

		class Handle {
			std::filesystem::path file;
		};
	public:
		virtual Handle Load(std::filesystem::path file) = 0;
		virtual void Store(const Handle &handle) = 0;
	};

}