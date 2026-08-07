#pragma once
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
	Mupfel::Entity												   e;
	std::unordered_map<std::string, Mupfel::ImageHandle>		   image_map;
	std::unordered_map<std::string, Mupfel::Animation>					   animations;
	/** Key into `animations` of the sequence currently playing; empty until the first selection. */
	std::string													   current_anim;
	bool														   moving_right = false;
	bool														   moving_left = false;
	bool														   moving_up = false;
	bool														   moving_down = false;
};
