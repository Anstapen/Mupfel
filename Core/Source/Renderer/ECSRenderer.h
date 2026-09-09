#pragma once
#include "Logger.h"
#include "SubRenderer.h"
#include <optional>

namespace Mupfel
{
class ECSRenderer : public SubRenderer
{
public:
	bool Init(nvrhi::DeviceHandle device, const nvrhi::FramebufferInfo& frameBufferInfo, uint32_t framesInFlight) final;
	void PreUser(nvrhi::DeviceHandle device, nvrhi::CommandListHandle current_command_list, const FrameContext& context)
		final;
	void PostUser(
		nvrhi::DeviceHandle		 device,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context) final;

private:
	Logger::SafeLoggerPtr logger;
	uint32_t			  currentImageCount = 0;
};

} // namespace Mupfel
