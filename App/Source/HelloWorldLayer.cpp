#include "HelloWorldLayer.h"
#include "Core/Application.h"
#include "Core/Profiler.h"
#include "ECS/View.h"

#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Animation.h"
#include "ECS/Components/Light.h"

#include "EditorLayer.h"

using namespace Mupfel;


void HelloWorldLayer::OnInit()
{
	auto result = Application::LoadBasicImage("Images/grass_1.png")
					  .transform([this](ImageHandle handle) { this->image_map["DefaultGrass"] = handle; });

	result = Application::LoadBasicImage("Images/ball_blue.png")
				 .transform([this](ImageHandle handle) { this->image_map["BlueBall"] = handle; });
	result = Application::LoadBasicImage("Images/ball_green.png")
				 .transform([this](ImageHandle handle) { this->image_map["GreenBall"] = handle; });
	result = Application::LoadBasicImage("Images/ball_default.png")
				 .transform([this](ImageHandle handle) { this->image_map["DefaultBall"] = handle; });
	result = Application::LoadBasicImage("Images/ball_red.png")
				 .transform([this](ImageHandle handle) { this->image_map["RedBall"] = handle; });

	result = Application::LoadAnimatedImage("Images/Soldier.png", {.rows = 7, .columns = 9})
				 .transform([this](ImageHandle handle) { this->image_map["Soldier"] = handle; });

	auto handles = Application::LoadSpriteSheetImages("Images/magecity.png", {.rows = 44, .columns = 8});

	if (handles.has_value())
	{
		spritesheet = handles.value();
	}

	std::vector<std::pair<uint32_t, uint32_t>> animations;

	animations.push_back({0, 6});
	animations.push_back({9, 8});
	animations.push_back({18, 6});
	animations.push_back({27, 6});
	animations.push_back({36, 9});
	animations.push_back({45, 4});
	animations.push_back({54, 4});

	Mupfel::Registry& registry = Application::GetCurrentRegistry();

	// Ground: one large flat quad in the x/y plane, grass tiled ~1 texture per world unit.
	{
		Entity	  e = registry.CreateEntity();
		Transform g;
		g.scale_x = 300.0f;
		g.scale_y = 300.0f;
		
		registry.AddComponent<Transform>(e, g);

		if (image_map.contains("DefaultGrass"))
		{
			Texture tex;
			tex.billboard = false;
			tex.uvScale = 300.0f;
			tex.index = image_map["DefaultGrass"];
			registry.AddComponent<Texture>(e, tex);
		}
	}

	{
		float pos_x = 0.0f;
		float pos_y = 0.0f;
		for (auto handle : spritesheet)
		{
			Entity	  e = registry.CreateEntity();
			Transform g;
			g.scale_x = 1.0f;
			g.scale_y = 1.0f;
			
			g.pos_x = pos_x;
			g.pos_y = pos_y;
			g.pos_z = 0.5f;
			registry.AddComponent<Transform>(e, g);

			Texture tex;
			tex.billboard = false;
			tex.uvScale = 1.0f;
			tex.index = handle;
			registry.AddComponent<Texture>(e, tex);

			pos_x += 1.0f;
			pos_y += 1.0f;
		}
	}

	// Player: an upright billboard at the origin; the camera follows this entity.
	{
		Entity	  e = registry.CreateEntity();
		Transform p;
		p.scale_x = 5.0f;
		p.scale_y = 5.0f;
		registry.AddComponent<Transform>(e, p);
		if (image_map.contains("Soldier"))
		{
			Texture tex;
			tex.index = image_map["Soldier"];
			registry.AddComponent<Texture>(e, tex);
		}

		Animation anim;
		anim.firstFrame = 0;
		anim.frameCount = 6;
		anim.fps = 6.0f;
		registry.AddComponent<Animation>(e, anim);
	}

	// A ring of billboard props; every third one floats above the plane.
	for (int i = 0; i < 12; i++)
	{
		Entity	  e = registry.CreateEntity();
		Transform t;
		t.pos_x = std::cos(i * 0.523f) * 15.0f;
		t.pos_y = std::sin(i * 0.523f) * 15.0f;
		t.pos_z = (i % 3 == 0) ? 4.0f : 0.0f;

		if (i == 3)
		{
			Light l;
			l.ambientStrength = 0.1f;
			l.r = 1.0f;
			l.g = 1.0f;
			l.b = 1.0f;
			registry.AddComponent<Light>(e, l);
		}

		registry.AddComponent<Transform>(e, t);

		if (image_map.contains("RedBall"))
		{
			Texture tex;
			tex.index = image_map["RedBall"];
			registry.AddComponent<Texture>(e, tex);
		}
	}
}

void HelloWorldLayer::OnUpdate(double timestep)
{
	Mupfel::Registry& registry = Application::GetCurrentRegistry();
	// Orbit the light(s) around the world origin on the x/y plane; radius matches the prop ring.
	constexpr float orbitRadius = 15.0f;
	constexpr float orbitSpeed = 1.0f; // radians per second
	constexpr float orbitHeight = 4.0f;

	lightOrbitAngle += orbitSpeed * timestep;

	for (auto [e, transform, light] : registry.view<Transform, Light>())
	{
		(void)e;
		(void)light;
		transform.pos_x = std::cos(lightOrbitAngle) * orbitRadius;
		transform.pos_y = std::sin(lightOrbitAngle) * orbitRadius;
		transform.pos_z = orbitHeight;
	}
}

void HelloWorldLayer::OnRender()
{
}


void HelloWorldLayer::ProcessEvents()
{

}
