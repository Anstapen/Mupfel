#include "EntityFileManager.h"
#include <fstream>
#include "ECS/Registry.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Collider.h"
#include "ECS/Components/Body.h"
#include "ECS/Components/Light.h"
#include "ECS/Components/Animation.h"
#include "ECS/Components/Sensor.h"
#include "json.hpp"

using namespace Mupfel;
using json = nlohmann::json;

Logger::SafeLoggerPtr EntityFileManager::logger = nullptr;

FileManager::Handle Mupfel::EntityFileManager::Load(const std::filesystem::path& file, ResourceManager* mngr)
{
    if (!logger)
    {
        logger = Logger::Create("EntityFileManager");
    }

    json data;

    if (mngr)
    {
        const std::shared_ptr<std::vector<uint8_t>> bytes = mngr->GetFile(file.string());
        if (!bytes)
        {
            logger->error("Unable to locate {} in the given resource file!", file.string());
            return {};
        }

        data = json::parse(bytes->begin(), bytes->end(), nullptr, false);
    }
    else if (Application::GetGlobalResourceManager())
    {
        const std::shared_ptr<std::vector<uint8_t>> bytes = Application::GetGlobalResourceManager()->GetFile(
            file.string());
        if (!bytes)
        {
            logger->error("Unable to locate {} in the global resource file!", file.string());
            return {};
        }

        data = json::parse(bytes->begin(), bytes->end(), nullptr, false);
    }
    else
    {
        std::ifstream file_stream(file);
        data = json::parse(file_stream, nullptr, false);
    }

    /* Check if the json parsing was successful. */
    if (data.empty() || data.is_discarded() || !data.is_array())
    {
        logger->error("Given JSON file {} is malformed or empty.", file.string());
        return {};
    }

    RegisterComponentLoader("Transform", EntityFileManager::LoadTransform);
    RegisterComponentLoader("Collider", EntityFileManager::LoadCollider);
    RegisterComponentLoader("Sensor", EntityFileManager::LoadSensor);
    RegisterComponentLoader("Texture", EntityFileManager::LoadTexture);
    RegisterComponentLoader("Body", EntityFileManager::LoadBody);
    RegisterComponentLoader("Light", EntityFileManager::LoadLight);
    RegisterComponentLoader("Animation", EntityFileManager::LoadAnimation);

    for (const auto& ent : data)
    {
        const auto it = ent.find("components");

        /* We do not create entities without components for now. */
        if (it == ent.end())
        {
            continue;
        }

        /* Create an Entity */
        const Entity e = Application::GetCurrentRegistry().CreateEntity();

        for (const auto& comp : *it)
        {
            const auto comp_name = comp.find("name");

            /* A component without a name is invalid. */
            if (comp_name == comp.end() || !comp_name->is_string())
            {
                continue;
            }
            auto loader = loaders.find(comp_name->get<std::string>());
            if (loader != loaders.end())
            {
                try
                {
                    loader->second(e, comp);
                }
                catch (const nlohmann::json::exception& ex)
                {
                    logger->error("Error when parsing json file: {}", ex.what());
                }
            }
        }
    }

    return {};
}

void Mupfel::EntityFileManager::Store(const FileManager::Handle& handle) { (void)handle; }

bool Mupfel::EntityFileManager::RegisterComponentLoader(const std::string& loader_name, ComponentLoader loader)
{
    auto [fst, snd] = loaders.insert({loader_name, loader});

    return snd;
}

void Mupfel::EntityFileManager::LoadTransform(Entity e, const nlohmann::json& source)
{
    Transform t;
    t.BuildFromJSON(source);
    Application::GetCurrentRegistry().AddComponent<Transform>(e, t);
}

void Mupfel::EntityFileManager::LoadCollider(Entity e, const nlohmann::json& source)
{
    Collider c;
    c.BuildFromJSON(source);
    Application::GetCurrentRegistry().AddComponent<Collider>(e, c);
}

void EntityFileManager::LoadSensor(Entity e, const nlohmann::json& source)
{
    Sensor s;
    s.BuildFromJSON(source);
    Application::GetCurrentRegistry().AddComponent<Sensor>(e, s);
}

void EntityFileManager::LoadTexture(Entity e, const nlohmann::json& source)
{
    Texture t;
    if (!t.BuildFromJSON(source))
    {
        logger->error("Could not build texture from json!");
    }

    Application::GetCurrentRegistry().AddComponent<Texture>(e, t);
}

void EntityFileManager::LoadBody(Entity e, const nlohmann::json& source)
{
    Body b;
    b.BuildFromJSON(source);
    Application::GetCurrentRegistry().AddComponent<Body>(e, b);
}

void EntityFileManager::LoadLight(Entity e, const nlohmann::json& source)
{
    Light l;
    l.BuildFromJSON(source);
    Application::GetCurrentRegistry().AddComponent<Light>(e, l);
}

void EntityFileManager::LoadAnimation(Entity e, const nlohmann::json& source)
{
    Animation a;
    a.BuildFromJSON(source);
    Application::GetCurrentRegistry().AddComponent<Animation>(e, a);
}
