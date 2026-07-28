#pragma once

#include "Core/Layer.h"
#include "Renderer/ImageManager.h"
#include <random>
#include "ECS/Entity.h"

class HelloWorldLayer : public Mupfel::Layer
{
	void OnInit() override;
	void OnUpdate(double timestep) override;
	void OnRender() override;
private:
	void ProcessEvents();
	void UpdatePlayerPosition(double timestep);
	void CheckRelevantEvents();
	void InitKeybinds();

private:
	std::unordered_map<std::string, Mupfel::ImageHandle> image_map;
	std::vector<Mupfel::ImageHandle> spritesheet;
	/** Shared RNG for `SpawnRandomEntities`. */
	std::mt19937 rng{std::random_device{}()};
	/** Accumulated orbit angle in radians for `UpdateLights`. */
	double lightOrbitAngle = 0.0f;
	Mupfel::Entity player;
	bool		   moving_right = false;
	bool		   moving_left = false;
	bool		   moving_up = false;
	bool		   moving_down = false;
};

