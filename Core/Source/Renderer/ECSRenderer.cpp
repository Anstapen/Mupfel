#include "ECSRenderer.h"
#include "CameraMath.h"
#include "Core/Application.h"
#include "ImageManager.h"
#include "Quad.h"

#include "ECS/Components/Animation.h"
#include "ECS/Components/Light.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Transform.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Shaders/ecs_fragment.h"
#include "Shaders/ecs_vertex.h"

struct UniformBuffer
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 cameraPos;
};

bool Mupfel::ECSRenderer::Init(
	nvrhi::DeviceHandle			  device,
	ImageManager&				  img_manager,
	const nvrhi::FramebufferInfo& frameBufferInfo,
	uint32_t					  framesInFlight)
{
	/* TODO: once the unrecoverable error handler is implemented, check this initialization! */
	(void)framesInFlight;
	(void)img_manager;
	logger = Logger::Create("ECS Renderer");

	nvrhi::ShaderHandle vertexShader = device->createShader(
		nvrhi::ShaderDesc().setShaderType(nvrhi::ShaderType::Vertex).setEntryName("vertMain"), ecsVertex,
		ecsVertex_sizeInBytes);

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
		nvrhi::ShaderDesc().setShaderType(nvrhi::ShaderType::Pixel).setEntryName("fragMain"), ecsFragment,
		ecsFragment_sizeInBytes);

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
						  .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(2))
						  .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(3))
						  .addItem(nvrhi::BindingLayoutItem::PushConstants(4, sizeof(uint32_t)))
						  .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(5));

	bindingLayout = device->createBindingLayout(layoutDesc);

	auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
							.setInputLayout(inputLayout)
							.addBindingLayout(bindingLayout)
							.setVertexShader(vertexShader)
							.setPixelShader(fragmentShader)
							.setRenderState(
								nvrhi::RenderState()
									.setDepthStencilState(
										nvrhi::DepthStencilState().enableDepthTest().enableDepthWrite().setDepthFunc(
											nvrhi::ComparisonFunc::LessOrEqual))
									.setRasterState(nvrhi::RasterState().setCullMode(nvrhi::RasterCullMode::None))
									.setBlendState(
										nvrhi::BlendState().setRenderTarget(
											0, nvrhi::BlendState::RenderTarget()
												   .enableBlend()
												   .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
												   .setDestBlend(nvrhi::BlendFactor::InvSrcAlpha)
												   .setSrcBlendAlpha(nvrhi::BlendFactor::One)
												   .setDestBlendAlpha(nvrhi::BlendFactor::Zero))));

	this->pipeline = device->createGraphicsPipeline(pipelineDesc, frameBufferInfo);

	if (!this->pipeline)
	{
		return false;
	}

	auto constantBufferDesc = nvrhi::BufferDesc()
								  .setByteSize(sizeof(UniformBuffer))
								  .setIsConstantBuffer(true)
								  .setIsVolatile(true)
								  .setMaxVersions(16);
	mvpBuffer = device->createBuffer(constantBufferDesc);

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

	/*
	 * Add some default light instances to not bind an empty buffer.
	 * The usage of the buffer is governed by the numLights parameter.
	 * That means the initial content of the light instance buffer does not matter.
	 */
	lightBuffer.PushBack({});
	if (lightBuffer.FlushToGPU(device, uploadList) == DualBufferStatus::BUFFER_ALLOCATION_FAILED)
	{
		uploadList->close();
		device->executeCommandList(uploadList);
		return false;
	}

	uploadList->close();
	device->executeCommandList(uploadList);

	return true;
}

void Mupfel::ECSRenderer::PreUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)img_manager;
	(void)context;

	UpdateMVP(current_command_list);
	SyncRenderableObjects();
	SyncLights();
}

void Mupfel::ECSRenderer::PostUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	if (!UploadDataToGPU(device, current_command_list))
	{
		return;
	}

	if (needDescriptorSetRebuild)
	{
		RebuildDescriptorSet(device, img_manager);
	}

	/* draw at max 2^32 entities. */
	drawableEntities =
		static_cast<uint32_t>(std::min<uint64_t>(transformBuffer.Size(), std::numeric_limits<uint32_t>::max()));

	if (drawableEntities == 0)
	{
		return;
	}

	/* Only sync if there is actually something to draw. */
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
	current_command_list->setPushConstants(&numLights, sizeof(numLights));
	current_command_list->drawIndexed(nvrhi::DrawArguments().setVertexCount(6).setInstanceCount(drawableEntities));
}

void Mupfel::ECSRenderer::UpdateMVP(nvrhi::CommandListHandle current_command_list)
{
	const float	  width = static_cast<float>(Application::GetCurrentRenderWidth());
	const float	  height = static_cast<float>(Application::GetCurrentRenderHeight());
	const Camera& cam = Application::GetCurrentSceneCamera();

	UniformBuffer ubo{};
	ubo.view = CameraMath::View(cam);
	ubo.proj = CameraMath::Projection(cam, width, height);
	ubo.cameraPos = glm::vec4(CameraMath::Eye(cam), 1.0f);

	current_command_list->writeBuffer(mvpBuffer, &ubo, sizeof(UniformBuffer));
}

void Mupfel::ECSRenderer::RebuildDescriptorSet(nvrhi::DeviceHandle device, ImageManager& img_manager)
{
	nvrhi::BindingSetDesc bindingSetDesc =
		nvrhi::BindingSetDesc()
			.addItem(nvrhi::BindingSetItem::Sampler(1, sampler))
			.addItem(nvrhi::BindingSetItem::ConstantBuffer(2, mvpBuffer))
			.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(3, transformBuffer.GetGPUBufferHandle()))
			.addItem(nvrhi::BindingSetItem::PushConstants(4, sizeof(uint32_t)))
			.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(5, lightBuffer.GetGPUBufferHandle()));

	for (uint32_t i = 0; i < MAX_IMAGE_COUNT; i++)
	{
		bindingSetDesc.addItem(
			nvrhi::BindingSetItem::Texture_SRV(0, img_manager.GetTextureHandle(i))
				.setDimension(nvrhi::TextureDimension::Texture2DArray)
				.setArrayElement(i));
	}

	bindingSet = device->createBindingSet(bindingSetDesc, bindingLayout);

	boundGeneration = img_manager.GetGeneration();
	needDescriptorSetRebuild = false;
}

bool Mupfel::ECSRenderer::OutOfSync(ImageManager& img_manager) const
{
	return boundGeneration != img_manager.GetGeneration();
}

bool Mupfel::ECSRenderer::UploadDataToGPU(nvrhi::DeviceHandle device, nvrhi::CommandListHandle current_command_list)
{
	if (transformBuffer.Empty())
	{
		/* We do not have anything to draw. */
		return true;
	}

	DualBufferStatus status = transformBuffer.FlushToGPU(device, current_command_list);

	if (status == DualBufferStatus::BUFFER_ALLOCATION_FAILED)
	{
		return false;
	}

	if (status == DualBufferStatus::BUFFER_RESIZED)
	{
		needDescriptorSetRebuild = true;
	}

	status = lightBuffer.FlushToGPU(device, current_command_list);

	if (status == DualBufferStatus::BUFFER_ALLOCATION_FAILED)
	{
		return false;
	}

	if (status == DualBufferStatus::BUFFER_RESIZED)
	{
		needDescriptorSetRebuild = true;
	}

	return true;
}

void Mupfel::ECSRenderer::SyncRenderableObjects()
{
	Mupfel::Registry& registry = Mupfel::Application::GetCurrentRegistry();

	transformBuffer.Clear();

	TextureInstance instance;

	for (auto [e, texture, transform] : registry.view<Mupfel::Texture, Mupfel::Transform>())
	{

		instance.index = texture.index;
		instance.scale_x = texture.scale_x;
		instance.scale_y = texture.scale_y;
		instance.pos_x = transform.pos_x;
		instance.pos_y = transform.pos_y;
		instance.pos_z = transform.pos_z;
		instance.rotation = transform.rotation;

		/* TODO: check if GetSignature might be more performant! */
		instance.emitsLight = registry.HasComponent<Mupfel::Light>(e) ? 1 : 0;

		if (registry.HasComponent<Mupfel::Animation>(e))
		{
			instance.frame = registry.GetComponent<Mupfel::Animation>(e).currentFrame;
		}
		else
		{
			instance.frame = 0;
		}

		transformBuffer.PushBack(instance);
	}
}

void Mupfel::ECSRenderer::SyncLights()
{
	Mupfel::Registry& registry = Mupfel::Application::GetCurrentRegistry();

	lightBuffer.Clear();

	LightInstance instance;

	for (auto [e, light, transform] : registry.view<Mupfel::Light, Mupfel::Transform>())
	{
		/* Populate the LightInstance object for the fragment shader */
		instance.ambientStrength = light.ambientStrength;
		instance.colorRGB.r = light.r;
		instance.colorRGB.g = light.g;
		instance.colorRGB.b = light.b;
		instance.pos.x = transform.pos_x;
		instance.pos.y = transform.pos_y;
		instance.pos.z = transform.pos_z;

		lightBuffer.PushBack(instance);
	}

	numLights = static_cast<uint32_t>(std::min<uint64_t>(lightBuffer.Size(), std::numeric_limits<uint32_t>::max()));
}