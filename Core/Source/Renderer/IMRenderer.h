#pragma once
#include "DualBuffer.h"
#include "ImageManager.h"
#include "Logger.h"
#include "SubRenderer.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Mupfel
{

/**
 * This class implements the SubRenderer interface to provide an Immediate Mode
 * Renderer to the user. Using this renderer, the user can draw several different
 * UI elements to the screen.
 */
class IMRenderer : public SubRenderer
{
public:
	/**
	 * This structure is used to hold render data per renderable item.
	 * It contains the following attributes:
	 * - 2D position
	 * - width and height of the item
	 * - rotation
	 * - texture index
	 * - scale of the texture
	 */
	struct TextureInstance
	{
		float	 pos_x = 0.0f;
		float	 pos_y = 0.0f;
		float	 width = 1.0f;
		float	 height = 1.0f;
		float	 rotation = 0.0f;
		uint32_t index = 1;
		float	 uvScale = 1.0f;
		float	 _pad0;
	};

public:
	/**
	 * Initialize the Immediate Mode Renderer. This function does the following:
	 *
	 * 1. Creating the graphics pipeline.
	 * 2. Creating CPU buffers and the decriptor set.
	 *
	 * \param device The NVRHI device used for buffer allocation.
	 * \param img_manager The image manager used to retrieve texture handles.
	 * \param frameBufferInfo The framebuffer information.
	 * \param framesInFlight Currently unused.
	 * \return True if the initialization was successful, false otherwise.
	 */
	bool Init(
		nvrhi::DeviceHandle			  device,
		ImageManager&				  img_manager,
		const nvrhi::FramebufferInfo& frameBufferInfo,
		uint32_t					  framesInFlight) final;

	/**
	 * Prepare a render pass.
	 * This clears the CPU side buffer of drawable items.
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
	 * Finish a render pass. This function does the following:
	 *
	 * 1. Uploads the render data of the current frame to the GPU.
	 * 2. Rebuilds the descriptor set, if needed.
	 * 3. Issues draw commands to draw the items.
	 *
	 * \param device The NVRHI device used for buffer allocation.
	 * \param img_manager The image manager used for descriptor set rebuilds.
	 * \param current_command_list The command list used to issue draw commands.
	 * \param context The frame information.
	 */
	void PostUser(
		nvrhi::DeviceHandle		 device,
		ImageManager&			 img_manager,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context) final;

	/**
	 * Display a button with the given texture.
	 *
	 * \param x x-offset in screen space.
	 * \param y y-offset in screen space.
	 * \param width The width of the button. This is independent of the used texture.
	 * \param height The height of the button. This is independent of the used texture.
	 * \param image_path Path to an image containing the button texture.
	 *
	 * \retval 0 if the cursor is not overlapping the button
	 * \retval 1 if the cursor is hovering over the button
	 * \retval 2 if the button is pressed.
	 */
	uint32_t Button(float x, float y, float width, float height, const std::string& image_path);

private:
	/**
	 * This pushes a renderable item into the item buffer.
	 *
	 * The interpretation of these values largely depends on the type of the item.
	 *
	 * \param x The x position of the item.
	 * \param y The y position of the item.
	 * \param width The width of the item.
	 * \param height The height of the item.
	 * \param rotation The rotation of the item.
	 * \param index The texture index.
	 * \param uv_scale The texture scale.
	 */
	void PushObject(float x, float y, float width, float height, float rotation, uint32_t index, float uv_scale);

	/**
	 * This function rebuilds the descriptor set if referenced buffers were resized or textures
	 * were added/removed.
	 *
	 * \param device The NVRHI device used for descriptor set allcoation.
	 * \param img_manager The image manager used to update the descriptor set.
	 */
	void RebuildDescriptorSet(nvrhi::DeviceHandle device, ImageManager& img_manager);

	/**
	 * This function checks whether or not the Immediate Mode Renderer is out of sync with the
	 * image manager. If yes, the descriptor set needs to be updated, so that the
	 * shaders can reference the correct images.
	 *
	 * \param img_manager The image manager to compare against.
	 * \return True if the IM renderer and given image manager are out of sync, false otherwise.
	 */
	bool OutOfSync(ImageManager& img_manager) const;

private:
	/** The logger object to log information. */
	Logger::SafeLoggerPtr logger;

	/** The graphics pipeline. */
	nvrhi::GraphicsPipelineHandle pipeline;

	/** The descriptor layout of the graphics pipeline. */
	nvrhi::BindingLayoutHandle bindingLayout;

	/** The vertex buffer. */
	nvrhi::BufferHandle		   vertexBuffer{};

	/**  The index buffer. */
	nvrhi::BufferHandle		   indexBuffer{};

	/** The descriptor set. */
	nvrhi::BindingSetHandle bindingSet;

	/** The sampler for the textures. */
	nvrhi::SamplerHandle	sampler;

	/** The current image manager generation. */
	uint32_t boundGeneration = 0;

	/** The number of drawable items this frame. */
	uint32_t drawableItems = 0;

	/** The buffer for per-item rendering data. */
	DualBuffer<TextureInstance> transformBuffer;
};
} // namespace Mupfel
