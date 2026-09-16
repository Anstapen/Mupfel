/*****************************************************************/
/**
 * \file   Renderer.h
 * \brief  The top level renderer used to prepare the render pass for the subrenderers.
 *
 * \author Anton Stapenhorst
 * \date   September 2026
 *****************************************************************/

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "Core/Window.h"
#include "DebugRenderer.h"
#include "GeometryRenderer.h"
#include "IMRenderer.h"
#include "Logger.h"
#include "NVRHIContext.h"
#include "SubRenderer.h"
#include "ImageManager.h"

namespace Mupfel
{

class UI;
class DebugLayer;

/**
 * This is the top level renderer. It does not issue any draw calls, instead it synchronizes CPU <-> GPU (via NVRHI
 * EventQueries) and GPU <-> presentation engine. Additionally it prepares the command list and calls the SubRenderer
 * objects, which actually do the drawing work.
 */
class Renderer
{
	friend class UI;
	friend class DebugLayer;

public:
	/**
	 * Initialize the renderer.
	 *
	 * If Begin() or End() are called without initializing, the renderer silently returns.
	 *
	 * \param window The window to which the renderer shall render to.
	 * \return True if the initialization was successful, false otherwise.
	 */
	bool Init(const Window& window);

	/**
	 * Stops rendering and frees all the related NVRHI and Vulkan data.
	 *
	 */
	void Shutdown();

	/**
	 * Prepares the current frame for rendering.
	 *
	 * This function waits until one of the frames is ready for rendering.
	 *
	 * \param window The window to render to.
	 * \param delta_time The time passed since the last frame.
	 */
	void Begin(const Window& window, double delta_time);

	/**
	 * Finishes the rendering of the current frame and submits it to the GPU.
	 *
	 * \param window The window to render to.
	 * \param delta_time The time passed since the last frame.
	 */
	void End(const Window& window, double delta_time);

	/**
	 * Retrieve the current image manager to (un-)load images.
	 * 
	 * \return The current image manager.
	 */
	ImageManager& GetImageManager();

private:
	/**
	 * Attempt to recreate the swapchain.
	 *
	 * This can be caused by multiple reasons, the most probable one being window resizing.
	 *
	 * \param window The current window.
	 * \return True if the swapchain recreation was successful, false otherwise.
	 */
	bool RecreateSwapchain(const Window& window);

	/**
	 * Increments the frameIndex safely, wrapping it around frames_in_flight.
	 *
	 */
	void incrementFrameIndex();

	/**
	 * (Re-)creates the NVRHI related swapchain textures and framebuffers.
	 *
	 * \return
	 */
	bool CreateSwapChainTexturesAndFramebuffers();

	/**
	 * Attempt to acquire the next swapchain image.
	 *
	 * On success, \ref imageIndex is updated with the new index and \a semaphore holds the semaphore to be used.
	 *
	 * \param window The current window.
	 * \param semaphore The semaphore to be used by the GPU to wait for the image to be ready.
	 * \return True if the next swapchain could be acquired, false otherwise.
	 */
	bool AcquireNextSwapchainImage(const Window& window, VkSemaphore& semaphore);

	/**
	 * Waits for the frame indicated by the current frameIndex.
	 *
	 * This function is used in conjunction with \ref MarkFrameSubmitted() to
	 * introduce a CPU wait barrier.
	 *
	 */
	void WaitForFrameSlot();

	/**
	 * Marks the frame indicated by the current frameIndex as "submitted".
	 *
	 * This function is used in conjunction with \ref WaitForFrameSlot() to
	 * introduce a CPU wait barrier.
	 *
	 */
	void MarkFrameSubmitted();

	/**
	 * Helper function to translate a VkFormat provided by vk-bootstrap (via the context member) into a
	 * nvrhi::Format.
	 *
	 * \return A nvrhi format that fits the vkFormat in context.swapchain.image_format.
	 */
	nvrhi::Format SelectFormat();

private:
	/** Standard "double-buffering" strategy. GPU related resources are duplicated and used in a ping-pong fashion. */
	static constexpr uint32_t frames_in_flight = 2;

	/** Initialization status. */
	bool isInitialized = false;

	/** This flag signals if the renderer was able to render the current frame. */
	bool frameValid = false;

	/** This flag signals whether or not the swapchain is usable.
	 *  An invalid swapchain prevents any subrenderer from doing stuff.
	 */
	bool isSwapchainUsable = false;

	/** The current frame index. Selects resources shared by GPU and CPU. */
	uint32_t frameIndex = 0;

	/** The current image index. Used select the image to draw to. */
	uint32_t imageIndex = 0;

	/** Logger object to display debug messages. */
	Logger::SafeLoggerPtr logger;

	/** Vulkan data structures needed to initialize a NVRHI device. */
	NVRHIContext context;

	/** The main NVRHI device. Used for the main NVRHI functionality
	 * (does not publish vulkan specific functions - use
	 * rawVKDevice for that).
	 */
	nvrhi::DeviceHandle nvrhiDevice;

	/** The NVRHI device interface for logical vulkan devices. Used for signalling and waiting semaphores. */
	nvrhi::RefCountPtr<nvrhi::vulkan::IDevice> rawVKDevice;

	/** An image manager to hold the images. */
	ImageManager imageManager;

	/** Textures for the Swapchain. */
	std::vector<nvrhi::TextureHandle> swapChainTextures;

	/** The texture for the depth buffer. */
	nvrhi::TextureHandle depthTexture;

	/** Framebuffers for the Swapchain. */
	std::vector<nvrhi::FramebufferHandle> frameBuffers;

	/** The command list to which all drawing commands are recorded. */
	nvrhi::CommandListHandle commandList;

	/** EventQueries that are used to synchronize the CPU and GPU. */
	std::array<nvrhi::EventQueryHandle, frames_in_flight> framesInFlightQueries;

	/** Array of all SubRenderers. These are invoked in the order they are pushed. */
	std::vector<std::shared_ptr<SubRenderer>> subRenderers;

	/**
	 * Functionality of these renderers can be directly invoked by the user,
	 * so we need to explicitly hold references to them.
	 */
	std::shared_ptr<IMRenderer>		  uiRenderer;
	std::shared_ptr<DebugRenderer>	  debugRenderer;
	std::shared_ptr<GeometryRenderer> geoRenderer;
};

} // namespace Mupfel
