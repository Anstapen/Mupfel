#pragma once
#include "Core/Logger.h"
#include "Mupfel.h"
#include <cstdint>
#include <string>
#include <unordered_map>

class Player
{
public:
	Player(Mupfel::Registry& registry);

public:
	void Init();
	void UpdateMovement(double timestep);

private:
	void CheckPlayerCollisions(void);

	void UpdatePlayerMovementSpeed(float speed);

private:
	Mupfel::Logger::SafeLoggerPtr						 logger;
	Mupfel::Entity										 e;
	std::unordered_map<std::string, Mupfel::Animation>	 animations;
	/** Key into `animations` of the sequence currently playing; empty until the first selection. */
	std::string current_anim;
	float		velocity_x = 0.0f;
	float		velocity_y = 0.0f;
	float		movement_speed = 3.0f;
};
