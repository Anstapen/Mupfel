#include "Renderer.h"
#include "Ping/Types.h"

#include "ECSRenderer.h"

using namespace Mupfel;

bool Mupfel::Renderer::Init(const Ping::Device& device, const Window& window)
{
	logger = Logger::Create("RenderingSystem");
	logger->info("Rendering System initializing...");

	/* The swapchain is owned by the rendering system */
	swapchain = device.CreateSwapChain(window.GetGLFWHandle(), frames_in_flight);

	if (!swapchain)
	{
		logger->error("Unable to create Swapchain!");
		return false;
	}

	depthBuffer = device.CreateDepthBuffer(swapchain.value());

	if (!depthBuffer)
	{
		logger->error("Unable to create Depth Buffer!");
		return false;
	}

	commandBuffers = device.CreateCommandBuffers(Ping::QueueType::Graphics, frames_in_flight);

	if (!commandBuffers)
	{
		logger->error("Unable to create Command Buffers!");
		return false;
	}

	/* Create the SubRenderers */
	subRenderers.emplace_back(std::move(std::make_unique<ECSRenderer>(frames_in_flight)));

	for (uint32_t i = 0; i < subRenderers.size(); i++)
	{
		if (!subRenderers[i]->Init(device, swapchain.value().GetFormat()))
		{
			return false;
		}
	}

	return true;
}

void Mupfel::Renderer::Begin(const Ping::Device& device, const Window& window, double delta_time)
{

	Ping::CommandBuffer& current_command_buffer = commandBuffers.value()[frameIndex];
	current_command_buffer.WaitForFences(device);

	imageIndex = swapchain.value().AcquireNextImage(frameIndex);

	/* Check if the index is valid */
	if (imageIndex == std::numeric_limits<uint32_t>::max())
	{
		/* Image was resized, swapchain needs to be recreated */
		swapchain.value().Recreate(device, window.GetGLFWHandle(), frames_in_flight);
		depthBuffer = device.CreateDepthBuffer(swapchain.value());
		return;
	}

	/* Start command recording */
	current_command_buffer.Begin(device, Ping::CommandBufferUsage::None);

	/* transition the current swapchain image to be a color attachment. */
	Ping::ImageLayoutTransition layout_transition = {
		.oldLayout = Ping::ImageLayout::Undefined,
		.newLayout = Ping::ImageLayout::ColorAttachmentOptimal,
		.srcAccessMask = Ping::AccessMask::None,
		.dstAccessMask = Ping::AccessMask::ColorAttachmentWrite,
		.srcStage = Ping::PipelineStage::ColorAttachmentOutput,
		.dstStage = Ping::PipelineStage::ColorAttachmentOutput,
		.aspect = Ping::ImageAspect::Color};

	current_command_buffer.transitionImageLayout(swapchain.value(), imageIndex, layout_transition);

	/* transition the depth buffer. */
	layout_transition.oldLayout = Ping::ImageLayout::Undefined;
	layout_transition.newLayout = Ping::ImageLayout::DepthAttachmentOptimal;
	layout_transition.srcAccessMask = Ping::AccessMask::DepthStencilAttachmentWrite;
	layout_transition.dstAccessMask = Ping::AccessMask::DepthStencilAttachmentWrite;
	layout_transition.srcStage = Ping::PipelineStage::EarlyFragmentTests | Ping::PipelineStage::LateFragmentTests;
	layout_transition.dstStage = Ping::PipelineStage::EarlyFragmentTests | Ping::PipelineStage::LateFragmentTests;
	layout_transition.aspect = Ping::ImageAspect::Depth;

	current_command_buffer.transitionImageLayout(depthBuffer.value(), layout_transition);

	/* start rendering to the swapchain image. */
	current_command_buffer.BeginRendering(swapchain.value(), depthBuffer.value(), imageIndex);

	for (uint32_t i = 0; i < subRenderers.size(); i++)
	{
		subRenderers[i]->PreUser(device, current_command_buffer);
	}
}

void Mupfel::Renderer::End(const Ping::Device& device, const Window& window, double delta_time)
{
	Ping::CommandBuffer& current_command_buffer = commandBuffers.value()[frameIndex];

	for (uint32_t i = 0; i < subRenderers.size(); i++)
	{
		subRenderers[i]->PostUser(device, current_command_buffer);
	}

	
	current_command_buffer.EndRendering();

	Ping::ImageLayoutTransition layout_transition = {
		.oldLayout = Ping::ImageLayout::ColorAttachmentOptimal,
		.newLayout = Ping::ImageLayout::PresentSource,
		.srcAccessMask = Ping::AccessMask::ColorAttachmentWrite,
		.dstAccessMask = Ping::AccessMask::None,
		.srcStage = Ping::PipelineStage::ColorAttachmentOutput,
		.dstStage = Ping::PipelineStage::BottomOfPipe,
		.aspect = Ping::ImageAspect::Color};

	current_command_buffer.transitionImageLayout(swapchain.value(), imageIndex, layout_transition);

	current_command_buffer.End();

	current_command_buffer.Submit(device, swapchain.value(), frameIndex, imageIndex);

	if (!swapchain.value().Present(device, imageIndex))
	{
		/* Image was resized, swapchain needs to be recreated */
		swapchain.value().Recreate(device, window.GetGLFWHandle(), frames_in_flight);
		depthBuffer = device.CreateDepthBuffer(swapchain.value());
	}

	incrementFrameIndex();
}

void Mupfel::Renderer::Shutdown() {}

void Mupfel::Renderer::incrementFrameIndex() { frameIndex = (frameIndex + 1) % frames_in_flight; }
