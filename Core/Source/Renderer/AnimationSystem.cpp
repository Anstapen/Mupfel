#include "AnimationSystem.h"
#include <cstdint>
#include "Core/Application.h"
#include "ECS/Components/Animation.h"

bool Mupfel::AnimationSystem::Init() { return true; }

void Mupfel::AnimationSystem::Update(double timestep) {
	Mupfel::Registry& registry = Mupfel::Application::GetCurrentRegistry();
	for (auto [e, animation] : registry.view<Animation>())
	{
		animation.elapsed += static_cast<float>(timestep);
		uint32_t step = static_cast<uint32_t>(animation.elapsed * animation.fps) % animation.frameCount;
		animation.currentFrame = animation.firstFrame + step;
	}
}

void Mupfel::AnimationSystem::Shutdown() {}
