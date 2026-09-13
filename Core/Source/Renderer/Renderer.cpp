/*********************************************************************/
/**
 * \file   Renderer.cpp
 * \brief  Implementation of Mupfel::Renderer.
 *
 * \author Anton Stapenhorst
 * \date   September 2026
 *********************************************************************/

#include "Renderer.h"
#include "Core/Application.h"
#include "ECSRenderer.h"

/* NVRHI and vk-bootstrap includes */
#include "NVRHIContext.h"
#include "nvrhi/utils.h"
#include "nvrhi/validation.h"
#include "nvrhi/vulkan.h"
#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

using namespace Mupfel;

/**
 * This class implements the IMessageCallback interface to print
 * NVRHI related messages. It uses the logger object given at
 * construction time.
 */
class CustomMessage : public nvrhi::IMessageCallback
{
public:
	CustomMessage() { logger = Logger::Create("NVRHI"); }

	void message(nvrhi::MessageSeverity severity, const char* messageText) final
	{
		switch (severity)
		{
		case nvrhi::MessageSeverity::Warning:
			logger->warn(messageText);
			break;
		case nvrhi::MessageSeverity::Error:
			logger->error(messageText);
			break;
		case nvrhi::MessageSeverity::Fatal:
			logger->critical(messageText);
			break;
		default:
			logger->info(messageText);
			break;
		}
	}

private:
	/* The shared pointer to the logger. */
	Logger::SafeLoggerPtr logger;
};

/**
 * A static CustomMessage object that is used to log NVRHI related information.
 */
static CustomMessage* msg = nullptr;

bool Renderer::Init(const Window& window)
{
	logger = Logger::Create("RenderingSystem");
	logger->info("Initializing Renderer...");

	int32_t width = 0;
	int32_t height = 0;
	window.GetFramebufferSize(width, height);

	/* This initializes the vulkan data structures that NVRHI device creation depends on. */
	if (!context.Init(window.GetGLFWHandle(), width, height, frames_in_flight))
	{
		return false;
	}

	nvrhi::vulkan::DeviceDesc deviceDesc;
	deviceDesc.instance = context.instance.instance;
	msg = new CustomMessage();
	deviceDesc.errorCB = msg;
	deviceDesc.physicalDevice = context.phys_device.physical_device;
	deviceDesc.device = context.device.device;
	deviceDesc.graphicsQueue = context.graphics_q;
	deviceDesc.graphicsQueueIndex = context.graphics_q_index;
	deviceDesc.transferQueue = context.transfer_q;
	deviceDesc.transferQueueIndex = context.transfer_q_index;
	deviceDesc.computeQueue = context.compute_q;
	deviceDesc.computeQueueIndex = context.compute_q_index;

	this->rawVKDevice = nvrhi::vulkan::createDevice(deviceDesc);
	this->nvrhiDevice = rawVKDevice;

	if (context.useValidation)
	{
		nvrhi::DeviceHandle nvrhiValidationLayer = nvrhi::validation::createValidationLayer(rawVKDevice);
		this->nvrhiDevice = nvrhiValidationLayer;
	}

	if (!CreateSwapChainTexturesAndFramebuffers())
	{
		Shutdown();
		return false;
	}

	/* All swapchain related data structures (semaphores, textures and framebuffers) are initialized. */
	this->isSwapchainUsable = true;

	/* Initialize the image manager. */
	if (!imageManager.Init(this->nvrhiDevice))
	{
		Shutdown();
		return false;
	}

	/* Create and push back all the SubRenderers. */
	subRenderers.push_back(std::make_shared<ECSRenderer>());
	uiRenderer = std::make_shared<IMRenderer>();
	subRenderers.push_back(uiRenderer);
	geoRenderer = std::make_shared<GeometryRenderer>();
	subRenderers.push_back(geoRenderer);
	debugRenderer = std::make_shared<DebugRenderer>();
	subRenderers.push_back(debugRenderer);

	for (uint32_t i = 0; i < subRenderers.size(); i++)
	{
		if (!subRenderers[i]->Init(nvrhiDevice, imageManager, frameBuffers[0]->getFramebufferInfo(), frames_in_flight))
		{
			Shutdown();
			return false;
		}
	}

	/* Create the command list and the eventqueries used to synchronize CPU and GPU. */
	commandList = nvrhiDevice->createCommandList();

	for (auto& query : framesInFlightQueries)
	{
		query = nvrhiDevice->createEventQuery();
		nvrhiDevice->resetEventQuery(query);
	}

	isInitialized = true;
	return true;
}

void Renderer::Begin(const Window& window, double delta_time)
{
	frameValid = false;

	if (!isInitialized)
	{
		return;
	}

	if (Application::IsWindowMinimized())
	{
		return;
	}

	if (!isSwapchainUsable)
	{
		/* Attempt to recreate the swapchain. */
		if (!RecreateSwapchain(window))
		{
			return;
		}
	}

	WaitForFrameSlot();

	VkSemaphore acquireSem = VK_NULL_HANDLE;

	if (!AcquireNextSwapchainImage(window, acquireSem))
	{
		/* This frame cannot be rendered. */
		return;
	}

	rawVKDevice->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, acquireSem, 0);

	frameValid = true;

	/* Draw Call recording phase. */

	commandList->open();

	nvrhi::utils::ClearColorAttachment(commandList, frameBuffers[imageIndex], 0, nvrhi::Color(0.1f, 0.1f, 0.12f, 1.0f));

	FrameContext frame{
		.frameBuffer = frameBuffers[imageIndex],
		.frameIndex = frameIndex,
		.width = context.swapchain.extent.width,
		.height = context.swapchain.extent.height,
		.deltaTime = delta_time,
	};

	for (auto& sr : subRenderers)
	{
		sr->PreUser(nvrhiDevice, imageManager, commandList, frame);
	}
}

void Renderer::End(const Window& window, double delta_time)
{
	if (!isInitialized)
	{
		return;
	}

	if (frameValid && isSwapchainUsable)
	{
		FrameContext frame{
			.frameBuffer = frameBuffers[imageIndex],
			.frameIndex = frameIndex,
			.width = context.swapchain.extent.width,
			.height = context.swapchain.extent.height,
			.deltaTime = delta_time,
		};

		for (auto& sr : subRenderers)
		{
			sr->PostUser(nvrhiDevice, imageManager, commandList, frame);
		}

		/* Present image to screen. */

		VkSemaphore presentSem = context.presentSemaphores[imageIndex];

		rawVKDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, presentSem, 0);

		commandList->close();
		this->nvrhiDevice->executeCommandList(commandList);

		VkPresentInfoKHR info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &presentSem;
		info.swapchainCount = 1;
		info.pSwapchains = &context.swapchain.swapchain;
		info.pImageIndices = &imageIndex;

		VkResult res = vkQueuePresentKHR(context.present_q, &info);

		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapchain(window);
		}

		MarkFrameSubmitted();
		incrementFrameIndex();
	}

	this->nvrhiDevice->runGarbageCollection();
}

ImageManager& Mupfel::Renderer::GetImageManager() { return imageManager; }

void Renderer::Shutdown()
{
	if (nvrhiDevice)
	{
		nvrhiDevice->waitForIdle();
	}

	/* Clear subrenderers. */
	subRenderers.clear();
	uiRenderer.reset();
	debugRenderer.reset();
	geoRenderer.reset();

	/* Shut down the image manager. */
	imageManager.Shutdown();

	/* Clear own NVRHI members. */
	commandList = nullptr;
	frameBuffers.clear();
	swapChainTextures.clear();

	if (nvrhiDevice)
	{
		nvrhiDevice->runGarbageCollection();
	}

	/* Clear the NVRHI device. */
	nvrhiDevice = nullptr;
	rawVKDevice = nullptr;

	delete msg;
	msg = nullptr;

	/* Destroy raw vulkan data. */
	context.Shutdown();

	isInitialized = false;
	isSwapchainUsable = false;
}

void Renderer::incrementFrameIndex() { frameIndex = (frameIndex + 1) % frames_in_flight; }

bool Renderer::CreateSwapChainTexturesAndFramebuffers()
{
	frameBuffers.clear();
	swapChainTextures.clear();
	nvrhiDevice->runGarbageCollection();

	/* Push the swapchain images */
	auto images_ret = context.swapchain.get_images();
	if (!images_ret)
	{
		/* There are no swapchain images. */
		return false;
	}

	const nvrhi::Format format = SelectFormat();
	if (format == nvrhi::Format::UNKNOWN)
	{
		logger->critical("Unsupported swapchain format: {}.", uint32_t(context.swapchain.image_format));
		return false;
	}

	const auto& images = images_ret.value();
	swapChainTextures.reserve(images.size());

	for (VkImage image : images)
	{
		auto textureDesc = nvrhi::TextureDesc()
							   .setDimension(nvrhi::TextureDimension::Texture2D)
							   .setFormat(SelectFormat())
							   .setWidth(context.swapchain.extent.width)
							   .setHeight(context.swapchain.extent.height)
							   .setIsRenderTarget(true)
							   .setDebugName("Swap Chain Image")
							   .enableAutomaticStateTracking(nvrhi::ResourceStates::Present);
		swapChainTextures.push_back(
			nvrhiDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, nvrhi::Object(image), textureDesc));

		auto framebufferDesc = nvrhi::FramebufferDesc().addColorAttachment(swapChainTextures.back());
		frameBuffers.push_back(nvrhiDevice->createFramebuffer(framebufferDesc));
	}

	return true;
}

bool Renderer::AcquireNextSwapchainImage(const Window& window, VkSemaphore& semaphore)
{
	VkResult res = VK_ERROR_UNKNOWN;

	/*
	 * Try to acquire the next image to render to.
	 * This algorithm is heavily inspired by
	 * https://github.com/NVIDIA-RTX/Donut/blob/main/src/app/vulkan/DeviceManager_VK.cpp.
	 */
	constexpr int maxAttempts = 3;
	for (int attempt = 0; attempt < maxAttempts; ++attempt)
	{
		semaphore = context.acquireSemaphores[context.acquireSemaphoreIndex];

		res = vkAcquireNextImageKHR(
			context.device.device, context.swapchain.swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &imageIndex);

		/* If VK_ERROR_OUT_OF_DATE_KHR got returned, we proceed with swapchain recreation. */
		if (res != VK_ERROR_OUT_OF_DATE_KHR || (attempt + 1 == maxAttempts))
		{
			break;
		}

		if (!RecreateSwapchain(window))
		{
			/* Swapchain recreation failed, but vulkan wants a new one. We cannot proceed to render this frame! */
			return false;
		}
	}

	/* If there was any other error with image acquisition, we cannot render this frame. */
	if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
	{
		return false;
	}

	/* Safely increment the semaphore index */
	context.acquireSemaphoreIndex = (context.acquireSemaphoreIndex + 1) % uint32_t(context.acquireSemaphores.size());

	return true;
}

bool Renderer::RecreateSwapchain(const Window& window)
{
	int32_t w = 0, h = 0;
	window.GetFramebufferSize(w, h);

	/* Check if the window is minimized. If yes, we skip recreation + rendering and try next frame. */
	if (w == 0 || h == 0)
	{
		return false;
	}

	this->isSwapchainUsable = false;

	if (!nvrhiDevice->waitForIdle())
	{
		logger->critical("Device lost before swapchain recreation.");
		return false;
	}

	frameBuffers.clear();
	swapChainTextures.clear();
	nvrhiDevice->runGarbageCollection();

	/* Attempt to rebuild the vulkan swapchain. */
	if (!context.CreateSwapchain(w, h, this->frames_in_flight))
	{
		logger->critical("Unable to recreate the swapchain.");
		return false;
	}

	/* Rebuild NVRHI related data from the new vulkan swapchain. */
	if (!CreateSwapChainTexturesAndFramebuffers())
	{
		logger->critical("Unable to recreate swapchain textures and framebuffers.");
		return false;
	}

	this->isSwapchainUsable = true;

	return true;
}

nvrhi::Format Renderer::SelectFormat()
{
	switch (context.swapchain.image_format)
	{
	case VK_FORMAT_B8G8R8A8_UNORM:
		return nvrhi::Format::BGRA8_UNORM;
	case VK_FORMAT_R8G8B8A8_UNORM:
		return nvrhi::Format::RGBA8_UNORM;
	case VK_FORMAT_B8G8R8A8_SRGB:
		return nvrhi::Format::SBGRA8_UNORM;
	case VK_FORMAT_R8G8B8A8_SRGB:
		return nvrhi::Format::SRGBA8_UNORM;
	default:
		return nvrhi::Format::UNKNOWN;
	}
}

void Renderer::WaitForFrameSlot() { nvrhiDevice->waitEventQuery(framesInFlightQueries[frameIndex]); }

void Renderer::MarkFrameSubmitted()
{
	nvrhiDevice->resetEventQuery(framesInFlightQueries[frameIndex]);
	nvrhiDevice->setEventQuery(framesInFlightQueries[frameIndex], nvrhi::CommandQueue::Graphics);
}