#include "EntityFileManager.h"
#include <fstream>
#include <iostream>
#include "ECS/Registry.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"
#include "Renderer/Texture.h"

using namespace Mupfel;
using json = nlohmann::json;

FileManager::Handle Mupfel::EntityFileManager::Load(std::filesystem::path file)
{
	json data;
	std::ifstream file_stream(file);
	try {
		data = json::parse(file_stream);
	}
	catch (...)
	{

	}

	RegisterComponentLoader("Transform", EntityFileManager::LoadTransform);
	RegisterComponentLoader("Texture", EntityFileManager::LoadTexture);
	RegisterComponentLoader("Movement", EntityFileManager::LoadMovement);
	RegisterComponentLoader("Collider", EntityFileManager::LoadCollider);

	for (auto ent : data)
	{

		/* Create an Entity */
		Entity e = Application::GetCurrentRegistry().CreateEntity();

		for (auto comp : ent["components"])
		{

			auto loader = loaders.find(comp["name"]);
			if (loader != loaders.end())
			{
				loader->second(e, comp);
			}
		}
	}
	
	return Handle();
}

void Mupfel::EntityFileManager::Store(const FileManager::Handle& handle)
{
}

bool Mupfel::EntityFileManager::RegisterComponentLoader(std::string loader_name, ComponentLoader loader)
{
	auto pair =  loaders.insert({ loader_name, loader });

	return pair.second;
}

void Mupfel::EntityFileManager::LoadTransform(Entity e, nlohmann::json source)
{
	Transform t;
	t.pos_x = source["pos_x"];
	t.pos_y = source["pos_y"];
	t.pos_z = source["pos_z"];
	t.scale_x = source["scale_x"];
	t.scale_y = source["scale_y"];
	t.rotation = source["rotation"];
	Application::GetCurrentRegistry().AddComponent<Transform>(e, t);
}

void Mupfel::EntityFileManager::LoadTexture(Entity e, nlohmann::json source)
{
	Application::GetCurrentRegistry().AddComponent<TextureComponent>(e, {});
}

void Mupfel::EntityFileManager::LoadMovement(Entity e, nlohmann::json source)
{
	Movement m;
	m.velocity_x = source["velocity_x"];
	m.velocity_y = source["velocity_y"];
	Application::GetCurrentRegistry().AddComponent<Movement>(e, m);
}

void Mupfel::EntityFileManager::LoadCollider(Entity e, nlohmann::json source)
{

	if (source["type"] == "Circle" && source.contains("radius"))
	{
		Mupfel::Collider col;
		col.SetCircle(source["radius"]);
		Application::GetCurrentRegistry().AddComponent<Mupfel::Collider>(e, col);
	}
	
}
