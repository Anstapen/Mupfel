#include "DebugRenderer.h"
#include "Core/Application.h"
#include "imgui.h"


bool Mupfel::DebugRenderer::Init(
	nvrhi::DeviceHandle			  device,
	ImageManager&				  img_manager,
	const nvrhi::FramebufferInfo& frameBufferInfo,
	uint32_t					  framesInFlight)
{
	(void)device;
	(void)img_manager;
	(void)frameBufferInfo;
	(void)framesInFlight;
	logger = Logger::Create("Debug Renderer");
	return true;
}

void Mupfel::DebugRenderer::PreUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)img_manager;
	(void)current_command_list;
	(void)context;
	/* TODO: draw Imgui */
}

void Mupfel::DebugRenderer::PostUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)img_manager;
	(void)current_command_list;
	(void)context;
}
