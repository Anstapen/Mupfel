#pragma once
#include "VkBootstrap.h"
#include "nvrhi/vulkan.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <vector>

namespace Mupfel
{

/**
 * This internal class encapsulates all structures needed by NVRHI.
 * For now, we just care about vulkan.
 */
class NVRHIContext
{
public:
	virtual ~NVRHIContext();
	bool Init(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t inFramesInFlight);
	void Shutdown();
	bool CreateSwapchain(uint32_t width, uint32_t height, uint32_t inFramesInFlight);

private:
	bool CreateInstanceAndSurface(GLFWwindow* window);
	bool PickPhysicalDevice();
	bool CreateLogicalDevice();
	bool CreateSemaphores(uint32_t image_count);
	void DestroySemaphores();

public:
#ifndef NDEBUG
	const bool useValidation = true;
#else
	const bool useValidation = false;
#endif
	vkb::Instance		instance{};
	vkb::PhysicalDevice phys_device{};
	vkb::Device			device{};
	VkSurfaceKHR		surface{};
	vkb::Swapchain		swapchain{};

	/* Queues */
	VkQueue	 graphics_q{};
	uint32_t graphics_q_index = 0;
	VkQueue	 transfer_q{};
	uint32_t transfer_q_index = 0;
	VkQueue	 compute_q{};
	uint32_t compute_q_index = 0;
	VkQueue	 present_q{};
	uint32_t present_q_index = 0;

	/** These semaphores are signaled by the GPU and waited for by the presentation engine. They state: "The GPU
	 *  finished writing to the related swapchain image and the presentation engine can display it."
	 * That means, each image presentation needs a semaphore.
	 */
	std::vector<VkSemaphore> presentSemaphores;

	/** These semaphores are signaled by the presentation engine and waited for by the GPU. They are used in
	 *  conjunction with a swapchain image and basically state: "When this semahore is signaled, the presentation of the
	 *  related image is finished and the GPU can start writing data to it."
	 *  That means, each queue submission (working on a specific image) needs one of those semaphores to wait until the
	 *  image is usable.
	 */
	std::vector<VkSemaphore> acquireSemaphores;

	/** The index of the current semaphore to use. This is incremented each frame. */
	uint32_t acquireSemaphoreIndex = 0;

	/** This value states how many frames in flight are used by the Renderer. Each frame in flight (i. e. GPU queue
	 *  submission) needs an acquireSemaphore to wait for image acquisition.
	 */
	uint32_t framesInFlight = 0;
};

} // namespace Mupfel