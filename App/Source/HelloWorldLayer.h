#pragma once

#include "Core/Layer.h"
#include "Renderer/ImageManager.h"
#include <random>

class HelloWorldLayer : public Mupfel::Layer
{
	void OnInit() override;
	void OnUpdate(double timestep) override;
	void OnRender() override;
private:
	void ProcessEvents();

private:
	std::unordered_map<std::string, Mupfel::ImageHandle> image_map;
	std::vector<Mupfel::ImageHandle> spritesheet;
	/** Shared RNG for `SpawnRandomEntities`. */
	std::mt19937 rng{std::random_device{}()};
	/** Accumulated orbit angle in radians for `UpdateLights`. */
	float lightOrbitAngle = 0.0f;
};

