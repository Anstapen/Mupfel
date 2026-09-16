#include "IMRenderer.h"
#include "Core/Application.h"
#include "Quad.h"
#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shaders/im_fragment.h"
#include "Shaders/im_vertex.h"

static const uint32_t default_entity_capacity = 1000;

bool Mupfel::IMRenderer::Init(
	nvrhi::DeviceHandle			  device,
	ImageManager&				  img_manager,
	const nvrhi::FramebufferInfo& frameBufferInfo,
	uint32_t					  framesInFlight)
{
	/* TODO: check error handling once the unrecoverable error handler is in place! */
	(void)framesInFlight;
	logger = Logger::Create("Immediate Mode Renderer");

	nvrhi::ShaderHandle vertexShader = device->createShader(
		nvrhi::ShaderDesc().setShaderType(nvrhi::ShaderType::Vertex).setEntryName("vertMain"), imVertex,
		imVertex_sizeInBytes);

	nvrhi::VertexAttributeDesc attributes[] = {
		nvrhi::VertexAttributeDesc()
			.setName("POSITION")
			.setFormat(nvrhi::Format::RG32_FLOAT)
			.setOffset(offsetof(Quad, pos))
			.setElementStride(sizeof(Quad)),
		nvrhi::VertexAttributeDesc()
			.setName("TEXCOORD")
			.setFormat(nvrhi::Format::RG32_FLOAT)
			.setOffset(offsetof(Quad, texCoord))
			.setElementStride(sizeof(Quad))};

	nvrhi::InputLayoutHandle inputLayout =
		device->createInputLayout(attributes, uint32_t(std::size(attributes)), vertexShader);

	nvrhi::ShaderHandle fragmentShader = device->createShader(
		nvrhi::ShaderDesc().setShaderType(nvrhi::ShaderType::Pixel).setEntryName("fragMain"), imFragment,
		imFragment_sizeInBytes);

	auto layoutDesc = nvrhi::BindingLayoutDesc()
						  .setVisibility(nvrhi::ShaderType::All)
						  .setBindingOffsets(
							  nvrhi::VulkanBindingOffsets()
								  .setShaderResourceOffset(0)
								  .setSamplerOffset(0)
								  .setConstantBufferOffset(0)
								  .setUnorderedAccessViewOffset(0))
						  .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0).setSize(MAX_IMAGE_COUNT))
						  .addItem(nvrhi::BindingLayoutItem::Sampler(1))
						  .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2));

	bindingLayout = device->createBindingLayout(layoutDesc);

	auto pipelineDesc =
		nvrhi::GraphicsPipelineDesc()
			.setInputLayout(inputLayout)
			.addBindingLayout(bindingLayout)
			.setVertexShader(vertexShader)
			.setPixelShader(fragmentShader)
			.setRenderState(
				nvrhi::RenderState()
					.setDepthStencilState(nvrhi::DepthStencilState().disableDepthTest().disableDepthWrite())
					.setRasterState(nvrhi::RasterState().setCullMode(nvrhi::RasterCullMode::None)));

	this->pipeline = device->createGraphicsPipeline(pipelineDesc, frameBufferInfo);

	if (!this->pipeline)
	{
		return false;
	}

	sampler = device->createSampler(
		nvrhi::SamplerDesc().setAllFilters(false).setAllAddressModes(nvrhi::SamplerAddressMode::ClampToEdge));

	auto vertexBufferDesc = nvrhi::BufferDesc()
								.setByteSize(quadVertices.size() * sizeof(Quad))
								.setIsVertexBuffer(true)
								.enableAutomaticStateTracking(nvrhi::ResourceStates::VertexBuffer)
								.setDebugName("Vertex Buffer");
	this->vertexBuffer = device->createBuffer(vertexBufferDesc);

	auto indexBufferDesc = nvrhi::BufferDesc()
							   .setByteSize(quadIndices.size() * sizeof(uint16_t))
							   .setIsIndexBuffer(true)
							   .enableAutomaticStateTracking(nvrhi::ResourceStates::IndexBuffer)
							   .setDebugName("Quad Index Buffer");
	this->indexBuffer = device->createBuffer(indexBufferDesc);

	/* Upload the buffer to the GPU. */
	nvrhi::CommandListHandle uploadList = device->createCommandList();
	uploadList->open();
	uploadList->writeBuffer(vertexBuffer, quadVertices.data(), quadVertices.size() * sizeof(Quad));
	uploadList->writeBuffer(indexBuffer, quadIndices.data(), quadIndices.size() * sizeof(uint16_t));

	/* lets push back a dummy so that the descriptor set is valid */
	transformBuffer.PushBack({});
	if (transformBuffer.FlushToGPU(device, uploadList) == DualBufferStatus::BUFFER_ALLOCATION_FAILED)
	{
		uploadList->close();
		device->executeCommandList(uploadList);
		return false;
	}

	uploadList->close();
	device->executeCommandList(uploadList);

	RebuildDescriptorSet(device, img_manager);

	return true;
}

void Mupfel::IMRenderer::PreUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)img_manager;
	(void)current_command_list;
	(void)context;
	transformBuffer.Clear();
}

void Mupfel::IMRenderer::PostUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	bool rebuild_descriptor_set = false;
	if (transformBuffer.Empty())
	{
		/* We do not have anything to draw. */
		return;
	}

	drawableItems =
		static_cast<uint32_t>(std::min<uint64_t>(transformBuffer.Size(), std::numeric_limits<uint32_t>::max()));

	DualBufferStatus status = transformBuffer.FlushToGPU(device, current_command_list);

	if (status == DualBufferStatus::BUFFER_ALLOCATION_FAILED)
	{
		return;
	}

	if (status == DualBufferStatus::BUFFER_RESIZED)
	{
		rebuild_descriptor_set = true;
	}

	if (rebuild_descriptor_set)
	{
		RebuildDescriptorSet(device, img_manager);
	}

	if (OutOfSync(img_manager))
	{
		RebuildDescriptorSet(device, img_manager);
	}

	nvrhi::GraphicsState state =
		nvrhi::GraphicsState()
			.setPipeline(pipeline)
			.setFramebuffer(context.frameBuffer)
			.setViewport(
				nvrhi::ViewportState().addViewportAndScissorRect(
					nvrhi::Viewport(float(context.width), float(context.height))))
			.addBindingSet(bindingSet)
			.addVertexBuffer(nvrhi::VertexBufferBinding().setBuffer(vertexBuffer).setSlot(0).setOffset(0))
			.setIndexBuffer(
				nvrhi::IndexBufferBinding().setBuffer(indexBuffer).setFormat(nvrhi::Format::R16_UINT).setOffset(0));

	current_command_list->setGraphicsState(state);
	current_command_list->drawIndexed(nvrhi::DrawArguments().setVertexCount(6).setInstanceCount(drawableItems));
}

uint32_t Mupfel::IMRenderer::Button(float x, float y, float width, float height, const std::string& image_path)
{
	if (Application::IsWindowMinimized())
	{
		return 0;
	}

	auto images_or_error = Application::LoadSpriteSheetImages(image_path, {.rows = 1, .columns = 3});

	if (!images_or_error)
	{
		return 0;
	}

	std::vector<uint32_t>& images = images_or_error.value();

	/* For the button to work correctly, we need at least 3 images. */
	if (images.size() < 3)
	{
		return 0;
	}

	ImageHandle image_h = images[0];

	uint32_t return_value = 0;

	double cursor_x = Application::GetCurrentInputManager().GetCurrentCursorX();
	double cursor_y = Application::GetCurrentInputManager().GetCurrentCursorY();

	/* Check collision with the button. */
	bool hovering = (cursor_x < x + width && cursor_x > x && cursor_y < y + height && cursor_y > y);

	if (hovering)
	{
		/* The button is released. TODO: this search is linear currently! */
		if (Application::GetCurrentInputManager().CheckUserInput(UserInput::LEFT_MOUSE_CLICK, KeyAction::RELEASED))
		{
			image_h = images[2];
			return_value = 3;
		}
		else if (Application::GetCurrentInputManager().CheckUserInput(UserInput::LEFT_MOUSE_CLICK, KeyAction::PRESSED))
		{
			image_h = images[2];
			return_value = 2;
		}
		else
		{
			image_h = images[1];
			return_value = 1;
		}
	}

	PushObject(x, y, width, height, 0.0f, image_h, 1.0f);

	return return_value;
}

void Mupfel::IMRenderer::PushObject(
	float	 x,
	float	 y,
	float	 width,
	float	 height,
	float	 rotation,
	uint32_t index,
	float	 uv_scale)
{
	const float screen_w = static_cast<float>(Application::GetCurrentRenderWidth());
	const float screen_h = static_cast<float>(Application::GetCurrentRenderHeight());

	if (screen_w <= 0.0f || screen_h <= 0.0f)
	{
		return;
	}

	/* The quad spans [-0.5, 0.5] around its centre, so convert the top-left pixel rect
	 * into an NDC centre plus an NDC extent. */
	TextureInstance t{};
	t.pos_x = ((x + width * 0.5f) / screen_w) * 2.0f - 1.0f;
	t.pos_y = ((y + height * 0.5f) / screen_h) * 2.0f - 1.0f;
	t.width = (width / screen_w) * 2.0f;
	t.height = (height / screen_h) * 2.0f;
	t.rotation = rotation;
	t.index = index;
	t.uvScale = uv_scale;

	transformBuffer.PushBack(t);
}

void Mupfel::IMRenderer::RebuildDescriptorSet(nvrhi::DeviceHandle device, ImageManager& img_manager)
{
	nvrhi::BindingSetDesc bindingSetDesc =
		nvrhi::BindingSetDesc()
			.addItem(nvrhi::BindingSetItem::Sampler(1, sampler))
			.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(2, transformBuffer.GetGPUBufferHandle()));

	for (uint32_t i = 0; i < MAX_IMAGE_COUNT; i++)
	{
		bindingSetDesc.addItem(
			nvrhi::BindingSetItem::Texture_SRV(0, img_manager.GetTextureHandle(i))
				.setDimension(nvrhi::TextureDimension::Texture2DArray)
				.setArrayElement(i));
	}

	bindingSet = device->createBindingSet(bindingSetDesc, bindingLayout);

	boundGeneration = img_manager.GetGeneration();
}

bool Mupfel::IMRenderer::OutOfSync(ImageManager& img_manager) const
{
	return boundGeneration != img_manager.GetGeneration();
}
