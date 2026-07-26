#pragma once
#include <cstdint>

namespace Mupfel
{
class Application;
class ECSRenderer;

struct Animation
{
	friend class Application;
	friend class ECSRenderer;
	uint32_t firstFrame = 0;
	uint32_t frameCount = 1;
	float	 fps = 10.0f;

private:
	float	 elapsed = 0.0f;
	uint32_t currentFrame = 0;
};
} // namespace Mupfel
