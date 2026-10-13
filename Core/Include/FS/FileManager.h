#pragma once
#include <filesystem>

namespace Mupfel
{
	class ResourceManager;

	class FileManager
	{
	public:
		virtual ~FileManager() = default;

		class Handle {
			std::filesystem::path file;
		};
	public:
		virtual Handle Load(const std::filesystem::path &file, ResourceManager * mngr) = 0;
		virtual void Store(const Handle &handle) = 0;
	};

}