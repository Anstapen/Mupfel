#pragma once
#include "Interactable.h"
#include "Mupfel.h"

class Spike : public Interactable
{
public:
	Spike(const std::string& texture_handle, float pos_x = 0.0f, float pos_y = 0.0f);
	virtual ~Spike();
	void CheckEvents() final;

private:
	Mupfel::Entity											  e;
	uint32_t												  current_visitors = 0;
	static std::unordered_map<std::string, Mupfel::Animation> animations;
	std::queue<const char*>									  animation_queue;
};
