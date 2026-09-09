#pragma once
#include "SubRenderer.h"

namespace Mupfel
{
class TriangleRenderer : public SubRenderer
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
	nvrhi::GraphicsPipelineHandle pipeline{};
	nvrhi::BufferHandle			  vertexBuffer{};
};
} // namespace Mupfel
