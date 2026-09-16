#pragma once
#include "Logger.h"
#include "SubRenderer.h"
#include "imgui.h"
#include <optional>

namespace Mupfel
{

/**
 * This class implements the SubRenderer interface to render DearImGui.
 */
class DebugRenderer : public SubRenderer
{
public:
	/**
	 * Initializes the subrenderer and DearImGui.
	 *
	 * 1. Creating an ImGui context and initializing vulkan.
	 * 2. Creating the graphics pipeline.
	 * 3. Creating the necessary GPU resources (sampler, font texture, descriptor set).
	 *
	 * \param device The NVRHI device used for resource creation.
	 * \param img_manager The current image manager, not used.
	 * \param frameBufferInfo The framebuffer information.
	 * \param framesInFlight The number of frames in flight, not used.
	 * \return True if the initialization was successful, false otherwise.
	 */
	bool Init(
		nvrhi::DeviceHandle			  device,
		ImageManager&				  img_manager,
		const nvrhi::FramebufferInfo& frameBufferInfo,
		uint32_t					  framesInFlight) final;

	/**
	 * Starts a new DearImGui frame.
	 * 
	 * \param device Currently unused.
	 * \param img_manager Currently unused.
	 * \param current_command_list Currently unused.
	 * \param context Currently unused.
	 */
	void PreUser(
		nvrhi::DeviceHandle		 device,
		ImageManager&			 img_manager,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context) final;

	/**
	 * Ends the DearImGui frame and issues the GPU draw calls.
	 * 
	 * \param device The NVRHI device used to allocate GPU resources.
	 * \param img_manager Currently unused.
	 * \param current_command_list The command list to issue draw calls to.
	 * \param context Information about the current frame.
	 */
	void PostUser(
		nvrhi::DeviceHandle		 device,
		ImageManager&			 img_manager,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context) final;

private:
	/**
	 * This function compares the current vertex buffer size with \a required_size.
	 * If the vertex buffer is too small, it uses \a device to allocate a new vertex buffer.
	 * The new vertex buffer is at least \a required_size large.
	 * 
	 * \param device The NVRHI device used for buffer allocation.
	 * \param required_size The required size of the vertex buffer in bytes.
	 * \return 
	 */
	bool CheckVertexBufferCapacity(nvrhi::DeviceHandle device, size_t required_size);

	/**
	 * This function compares the current index buffer size with \a required_size.
	 * If the index buffer is too small, it uses \a device to allocate a new index buffer.
	 * The new index buffer is at least \a required_size large.
	 *
	 * \param device The NVRHI device used for buffer allocation.
	 * \param required_size The required size of the index buffer in bytes.
	 * \return
	 */
	bool CheckIndexBufferCapacity(nvrhi::DeviceHandle device, size_t required_size);

private:
	/** A logger that can be used to provide messages to the user. */
	Logger::SafeLoggerPtr logger;

	/** The graphics pipeline. */
	nvrhi::GraphicsPipelineHandle pipeline;

	/** The binding layout of the graphics pipeline. */
	nvrhi::BindingLayoutHandle bindingLayout;

	/** The vertex buffer. */
	nvrhi::BufferHandle			  vertexBuffer{};

	/** The current vertex buffer size in bytes. */
	size_t						  vertexBufferSize = 0;

	/** The index buffer. */
	nvrhi::BufferHandle			  indexBuffer{};

	/**  The current index buffer size in bytes. */
	size_t						  indexBufferSize = 0;

	/** The desciptor set for the pipeline. */
	nvrhi::BindingSetHandle	   bindingSet;

	/** The DearImGui font atlas. */
	nvrhi::TextureHandle	   fontImage;

	/** The image sampler for the font atlas. */
	nvrhi::SamplerHandle	   sampler;
	
	/** The DearImGui context. */
	ImGuiContext*			   imguiContext;
};
} // namespace Mupfel
