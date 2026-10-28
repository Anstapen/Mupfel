#pragma once
#include "ECS/Components/Animation.h"

namespace Mupfel
{
class AnimationSystem
{
public:
	static bool Init();
	static void Update(double timestep);
	static void Shutdown();
private:
	static bool IsAnimationFinished(const Animation &animation);
};
} // namespace Mupfel
