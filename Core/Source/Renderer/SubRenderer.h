#pragma once
#include "nvrhi/nvrhi.h"
#include <cstdint>

namespace Mupfel
{

/**
 * This structure encapsulates frame information needed by the subrenderer.
 */
struct FrameContext
{
	/** The framebuffer for the image of the current frame. */
	nvrhi::IFramebuffer* frameBuffer = nullptr;

	/**
	 * The current frameIndex. The subrenderer can use this index to select
	 * the GPU resources to write this frame.
	 */
	uint32_t frameIndex = 0;

	/** The current width of the image. */
	uint32_t width = 0;

	/** The current height of the image. */
	uint32_t height = 0;

	/** The time since the last frame. */
	double deltaTime = 0.0;
};

/**
 * This interface can be used to implement a new Subrenderer.
 */
class SubRenderer
{
public:
	/**
	 * Initialize the subrenderer.
	 *
	 * \param device NVRHI device handle for which to create the subrenderer.
	 * \param frameBufferInfo The framebuffer information used for pipeline creation.
	 * \param framesInFlight The number of frames in flight. Each subrenderer is expected to create N copies of its
	 * internal GPU structures.
	 * \return True if the subrenderer initialization was successful, false otherwise.
	 */
	virtual bool
	Init(nvrhi::DeviceHandle device, const nvrhi::FramebufferInfo& frameBufferInfo, uint32_t framesInFlight) = 0;

	/**
	 * Do work before any user layers are rendered.
	 * 
	 * \param device NVRHI device that is used to render.
	 * \param current_command_list The command list to which the draw calls can be submitted.
	 * \param context The frame context.
	 */
	virtual void
	PreUser(nvrhi::DeviceHandle device, nvrhi::CommandListHandle current_command_list, const FrameContext& context) = 0;

	/**
	 * Do work after any user layers are rendered.
	 * 
	 * \param device NVRHI device that is used to render.
	 * \param current_command_list The command list to which the draw calls can be submitted.
	 * \param context The frame context.
	 */
	virtual void PostUser(
		nvrhi::DeviceHandle		 device,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context) = 0;
};
} // namespace Mupfel
