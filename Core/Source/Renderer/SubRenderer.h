#pragma once
#include "Ping/Device.h"
#include "Ping/Types.h"
#include "Ping/CommandBuffer.h"

namespace Mupfel
{
class SubRenderer
{
public:
	SubRenderer(uint32_t frames_in_flight);
	virtual ~SubRenderer() = default;
	/**
	 * Initialize the Renderer.
	 *
	 * \param swapChainFormat The format of the swapchain format.
	 * \return Whether or not the Init succeeded.
	 */
	virtual bool Init(const Ping::Device& device, Ping::Format swapChainFormat) = 0;

	/**
	 * This function is executed by the main renderer every frame before user code is executed.
	 *
	 */
	virtual void PreUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer) = 0;

	/**
	 * This function is executed by the main renderer every frame after user code was executed.
	 *
	 */
	virtual void PostUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer) = 0;

protected:
	void IncrementFrameIndex();

protected:
	/** The total frames in flight. The implementer is expected to allocate each of its buffers for all frames in
	 *  flight, to ensure exclusive access by the GPU.
	 */
	uint32_t framesInFlight;

	/** The index of the current frame, used to select the correct buffer. */
	uint32_t frameIndex;
};
} // namespace Mupfel
