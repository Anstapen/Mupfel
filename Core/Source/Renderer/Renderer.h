#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "Core/Window.h"
#include "DebugRenderer.h"
#include "IMRenderer.h"
#include "GeometryRenderer.h"
#include "Logger.h"
#include "Ping/Device.h"
#include "SubRenderer.h"

namespace Mupfel
{

class UI;
class DebugLayer;

class Renderer
{
	friend class UI;
	friend class DebugLayer;

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
	std::vector<std::shared_ptr<SubRenderer>> subRenderers;
	/* We hold a shared explicitly typed reference for this renderer, as the user needs to call it directly. */
	std::shared_ptr<IMRenderer>	   uiRenderer;
	std::shared_ptr<DebugRenderer> debugRenderer;
	std::shared_ptr<GeometryRenderer> geoRenderer;
};

} // namespace Mupfel
