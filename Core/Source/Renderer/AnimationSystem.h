#pragma once

namespace Mupfel
{
class AnimationSystem
{
public:
	bool Init();
	void Update(double timestep);
	void Shutdown();
};
} // namespace Mupfel
