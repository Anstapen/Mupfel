#pragma once

#include "FileManager.h"
#include "ECS/Entity.h"
#include <string>
#include <unordered_map>
#include "json.hpp"

namespace Mupfel {

	class EntityFileManager : public FileManager
	{
	public:
		typedef void (*ComponentLoader)(Entity e, nlohmann::json source);
	public:
		virtual Handle Load(std::filesystem::path file) override;
		virtual void Store(const Handle& handle) override;
		virtual bool RegisterComponentLoader(std::string loader_name, ComponentLoader loader);
	private:
		static void LoadTransform(Entity e, nlohmann::json source);
		static void LoadTexture(Entity e, nlohmann::json source);
		static void LoadMovement(Entity e, nlohmann::json source);
		static void LoadCollider(Entity e, nlohmann::json source);
	private:
		std::unordered_map<std::string, ComponentLoader> loaders;
	};

}

