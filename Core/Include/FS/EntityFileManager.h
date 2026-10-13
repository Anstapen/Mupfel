#pragma once

#include "FileManager.h"
#include "ECS/Entity.h"
#include "Core/Logger.h"
#include <string>
#include <functional>
#include <unordered_map>
#include "json_fwd.hpp"

namespace Mupfel
{
    class EntityFileManager : public FileManager
    {
    public:
        using ComponentLoader = std::function<void(Entity, const nlohmann::json&)>;

    public:
        Handle Load(const std::filesystem::path& file, ResourceManager* mngr) final;
        void Store(const Handle& handle) final;
        virtual bool RegisterComponentLoader(const std::string& loader_name, ComponentLoader loader);

    private:
        static void LoadTransform(Entity e, const nlohmann::json &source);
        static void LoadCollider(Entity e, const nlohmann::json &source);
        static void LoadSensor(Entity e, const nlohmann::json &source);
        static void LoadTexture(Entity e, const nlohmann::json &source);
        static void LoadBody(Entity e, const nlohmann::json &source);
        static void LoadLight(Entity e, const nlohmann::json &source);
        static void LoadAnimation(Entity e, const nlohmann::json &source);

    private:
        std::unordered_map<std::string, ComponentLoader> loaders;
        static Mupfel::Logger::SafeLoggerPtr logger;
    };
}
