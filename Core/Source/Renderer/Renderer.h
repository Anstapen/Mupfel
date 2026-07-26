#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "Core/Window.h"
#include "Logger/Logger.h"
#include "Ping/Device.h"
#include "SubRenderer.h"

namespace Mupfel
{

class Renderer
{
public:
	bool Init(const Ping::Device& device, const Window& window);

	void Begin(const Ping::Device& device, const Window& window, double delta_time);

	void End(const Ping::Device& device, const Window& window, double delta_time);

	void Shutdown();

private:
	void incrementFrameIndex();

private:
	static constexpr uint32_t				  frames_in_flight = 2;
	uint32_t								  frameIndex = 0;
	uint32_t								  imageIndex = 0;
	Logger::SafeLoggerPtr					  logger;
	std::optional<Ping::SwapChain>			  swapchain;
	std::optional<Ping::CommandBuffers>		  commandBuffers;
	std::optional<Ping::Image>				  depthBuffer;
	std::vector<std::unique_ptr<SubRenderer>> subRenderers;
};

} // namespace Mupfel
