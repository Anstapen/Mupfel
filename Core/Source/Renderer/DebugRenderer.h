#pragma once
#include "Logger.h"
#include "SubRenderer.h"
#include <optional>

namespace Mupfel
{
class DebugRenderer : public SubRenderer
{
public:
	bool Init(
		nvrhi::DeviceHandle			  device,
		ImageManager&				  img_manager,
		const nvrhi::FramebufferInfo& frameBufferInfo,
		uint32_t					  framesInFlight) final;
	void PreUser(
		nvrhi::DeviceHandle		 device,
		ImageManager&			 img_manager,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context)
		final;
	void PostUser(
		nvrhi::DeviceHandle		 device,
		ImageManager&			 img_manager,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context) final;

private:
	Logger::SafeLoggerPtr logger;
};
} // namespace Mupfel
