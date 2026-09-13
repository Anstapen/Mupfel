#pragma once
#include "Logger.h"
#include "SubRenderer.h"
#include "nvrhi/nvrhi.h"

namespace Mupfel
{
class ECSRenderer : public SubRenderer
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
		const FrameContext&		 context) final;
	void PostUser(
		nvrhi::DeviceHandle		 device,
		ImageManager&			 img_manager,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context) final;

private:
	void UpdateMVP(nvrhi::CommandListHandle current_command_list);
	void RebuildDescriptorSet(nvrhi::DeviceHandle device, ImageManager& img_manager);
	bool OutOfSync(ImageManager& img_manager) const;

private:
	/** Logger to print out information. */
	Logger::SafeLoggerPtr logger;

	nvrhi::GraphicsPipelineHandle pipeline;
	nvrhi::BufferHandle			  vertexBuffer{};
	nvrhi::BufferHandle			  indexBuffer{};
	nvrhi::BufferHandle			  mvpBuffer{};
	nvrhi::BindingSetHandle		  bindingSet;
	nvrhi::SamplerHandle		  sampler;
	nvrhi::BindingLayoutHandle	  bindingLayout;
	uint32_t					  boundGeneration = 0;
};

} // namespace Mupfel
