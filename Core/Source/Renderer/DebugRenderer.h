#pragma once
#include "SubRenderer.h"
#include "Ping/Gui.h"
#include <optional>
#include "Logger.h"


namespace Mupfel
{
class DebugRenderer : public SubRenderer
{
public:
	DebugRenderer(uint32_t frames_in_flight);

public:
	bool Init(const Ping::Device& device, Ping::Format swapChainFormat) final;
	void PreUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer) final;
	void PostUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer) final;
private:
	std::optional<Ping::Gui> gui;
	Logger::SafeLoggerPtr	 logger;
};
}


