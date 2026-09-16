#pragma once
#include "DualBuffer.h"
#include "Logger.h"
#include "SubRenderer.h"
#include "glm/glm.hpp"
#include "nvrhi/nvrhi.h"

namespace Mupfel
{

/**
 * This subrenderer is responsible to render entities that have certain components.
 * In the Mupfel ECS, entities are drawn automatically when they have a specific set of components.
 *
 * Currently, the following rules apply:
 *
 * 1. For an entity to be rendered, it must have the Transform and Texture components.
 * 2. The presence of an Animation component enables the entity to be animated
 *    (i.e. different layers of the texture are displayed.).
 * 3. The presence of a Light component makes the entity emit light.
 */
class ECSRenderer : public SubRenderer
{
public:
	/**
	 * This structure represents one drawable entity.
	 * It contains the following attributes:
	 * - 3D position
	 * - 2D scale
	 * - rotation
	 * - texture index
	 * - whether or not the entity emits light (i.e. has a Light component)
	 * - the frame index (if the entity is being animated)
	 */
	struct TextureInstance
	{
		float	 pos_x = 0.0f;
		float	 pos_y = 0.0f;
		float	 pos_z = 0.0f;
		float	 _pad0 = 0.0f;
		float	 scale_x = 1.0f;
		float	 scale_y = 1.0f;
		float	 rotation = 0.0f;
		uint32_t index = 1;
		float	 _pad1 = 0.0f;
		uint32_t emitsLight = 0;
		uint32_t frame = 0;
		float	 _pad2 = 0.0f;
	};

	/**
	 * This structure contains light information for one light.
	 * - the color of the light
	 * - the ambient strength of the light
	 * - the position of the light
	 */
	struct LightInstance
	{
		glm::vec3 colorRGB;
		float	  ambientStrength = 0.0f;
		glm::vec3 pos;
		float	  _pad1;
	};

public:
	/**
	 * This function initializes the ECS renderer.
	 *
	 * 1. The graphics pipeline (with vertex and fragment shaders, binding layouts) is created.
	 * 2. The buffers are created.
	 * 3. For buffers that may be empty at drawing time, default objects are inserted to
	 *    not prevent descriptor set creation.
	 *
	 * \param device The NVRHI device used to allocate GPU resources.
	 * \param img_manager The image manager used to retrieve the texture handles.
	 * \param frameBufferInfo The framebuffer information.
	 * \param framesInFlight The number of frames in flight. Currently unused.
	 * \return True if the initialization was successful, false otherwise.
	 */
	bool Init(
		nvrhi::DeviceHandle			  device,
		ImageManager&				  img_manager,
		const nvrhi::FramebufferInfo& frameBufferInfo,
		uint32_t					  framesInFlight) final;

	/**
	 * Prepare the next render pass. This function does the following:
	 *
	 * 1. Update the Model-View-Perpective buffer based on the current camera.
	 * 2. Determine which entities should be drawn this frame and gather
	 *    the needed information in a CPU-side buffer.
	 * 3. Determine which entities emit light this frame and gather the
	 *    needed data.
	 *
	 * \param device Currently unused.
	 * \param img_manager Currently unused.
	 * \param current_command_list The command list used to upload the MVP data.
	 * \param context Currently unused.
	 */
	void PreUser(
		nvrhi::DeviceHandle		 device,
		ImageManager&			 img_manager,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context) final;

	/**
	 * Finishes the current render pass. This function does the following:
	 *
	 * 1. Upload the data gathered in PreUser() to the GPU.
	 * 2. Rebuild the descriptor sets if the buffers were resized or textures
	 *    were added/removed.
	 * 3. Set the graphics state and issue indexed draw calls.
	 *
	 * \param device The NVRHI device used for buffer allocation.
	 * \param img_manager The image manager used to retrieve the latest image data.
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
	 * This function retrieves camera data (view, projection, camera position) and uploads it to the GPU.
	 *
	 * \param current_command_list The command list to use for GPU uploads.
	 */
	void UpdateMVP(nvrhi::CommandListHandle current_command_list);

	/**
	 * This function rebuilds the descriptor set if referenced buffers were resized or textures
	 * were added/removed.
	 *
	 * \param device The NVRHI device used for descriptor set allcoation.
	 * \param img_manager The image manager used to update the descriptor set.
	 */
	void RebuildDescriptorSet(nvrhi::DeviceHandle device, ImageManager& img_manager);

	/**
	 * This function checks whether or not the ECS renderer is out of sync with the
	 * image manager. If yes, the descriptor set needs to be updated, so that the
	 * shaders can reference the correct images.
	 *
	 * \param img_manager The image manager to compare against.
	 * \return True if the ECS renderer and given image manager are out of sync, false otherwise.
	 */
	bool OutOfSync(ImageManager& img_manager) const;

	/**
	 * This function gathers information about entities that are able to be rendered.
	 */
	void SyncRenderableObjects();

	/**
	 * This function gathers information about entities that are able to be rendered as lights.
	 *
	 */
	void SyncLights();

	/**
	 * This function attempts to upload the required render data (texture instances and light instances)
	 * to the GPU.
	 *
	 * \param device The device to be used for buffer allocation.
	 * \param current_command_list The command list to be used for buffer upload.
	 * \return True if the data was successfully uploaded to the GPU, false otherwise.
	 */
	bool UploadDataToGPU(nvrhi::DeviceHandle device, nvrhi::CommandListHandle current_command_list);

private:
	/** Logger to print out information. */
	Logger::SafeLoggerPtr logger;

	/** The graphics pipeline. */
	nvrhi::GraphicsPipelineHandle pipeline;

	/** The descriptor set layout for the graphics pipeline. */
	nvrhi::BindingLayoutHandle bindingLayout;

	/** The vertex buffer. */
	nvrhi::BufferHandle vertexBuffer{};

	/** The index buffer. */
	nvrhi::BufferHandle indexBuffer{};

	/** The constant buffer used to hold the view projection and camera position.  */
	nvrhi::BufferHandle mvpBuffer{};

	/** The descriptor set. */
	nvrhi::BindingSetHandle bindingSet;

	/** The sampler for the textures. */
	nvrhi::SamplerHandle sampler;

	/** The current image manager generation. */
	uint32_t boundGeneration = 0;

	/** The number of drawable entities this frame. */
	uint32_t drawableEntities = 0;

	/** The number of drawable lights this frame. */
	uint32_t numLights = 0;

	/** Boolean to keep track if the descriptor set needs to be rebuilt next frame. */
	bool needDescriptorSetRebuild = false;

	/** The buffer for per-entity rendering data. */
	DualBuffer<TextureInstance> transformBuffer;

	/** The buffer for light data. */
	DualBuffer<LightInstance>	lightBuffer;
};

} // namespace Mupfel
