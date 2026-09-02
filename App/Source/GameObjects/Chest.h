#pragma once
#include "Mupfel.h"
#include "Interactable.h"
#include <cstdint>
#include <queue>
#include <string>
#include <unordered_map>

/**
 * This class represents a chest in the game.
 * The "chest-like" behavior is achieved through the composition
 * of various components, such as a Collider, Sensor etc.
 */
class Chest : public Interactable
{
public:
	/**
	 * Create a chest.
	 *
	 */
	Chest(const std::string& texture_handle, float pos_x = 0.0f, float pos_y = 0.0f);
	virtual ~Chest();
	void CheckEvents() final;

private:
	Mupfel::Entity											  e;
	uint32_t												  current_visitors = 0;
	static std::unordered_map<std::string, Mupfel::Animation> animations;
	std::queue<const char*>									  animation_queue;
};
