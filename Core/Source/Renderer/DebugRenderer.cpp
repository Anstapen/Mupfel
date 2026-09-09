#include "DebugRenderer.h"
#include "Core/Application.h"
#include "imgui.h"


bool Mupfel::DebugRenderer::Init(
	nvrhi::DeviceHandle			  device,
	const nvrhi::FramebufferInfo& frameBufferInfo,
	uint32_t					  framesInFlight)
{
	(void)device;
	(void)frameBufferInfo;
	(void)framesInFlight;
	logger = Logger::Create("Debug Renderer");
	return true;
}

void Mupfel::DebugRenderer::PreUser(
	nvrhi::DeviceHandle		 device,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)current_command_list;
	(void)context;
	/* TODO: draw Imgui */
}

void Mupfel::DebugRenderer::PostUser(
	nvrhi::DeviceHandle		 device,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)current_command_list;
	(void)context;
}
