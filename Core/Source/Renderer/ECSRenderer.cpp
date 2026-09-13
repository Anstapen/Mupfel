#include "ECSRenderer.h"
#include "CameraMath.h"
#include "Core/Application.h"
#include "ImageManager.h"
#include "Quad.h"

#include "ECS/Components/Animation.h"
#include "ECS/Components/Light.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Transform.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shaders/ecs_fragment.h"
#include "Shaders/ecs_vertex.h"

struct UniformBuffer
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 cameraPos;
};

struct TextureInstance
{
	float	 pos_x = 0.0f;
	float	 pos_y = 0.0f;
	float	 pos_z = 0.0f;
	float	 _pad0;
	float	 scale_x = 1.0f;
	float	 scale_y = 1.0f;
	float	 rotation = 0.0f;
	uint32_t index = 1;
	float	 _pad1;
	uint32_t emitsLight = 0;
	uint32_t frame = 0;
	float	 _pad2;
};

struct LightInstance
{
	glm::vec3 colorRGB;
	float	  ambientStrength = 0.0f;
	glm::vec3 pos;
	float	  _pad1;
};

struct LightParams
{
	uint32_t numLights = 0;
	float	 _pad0 = 0.0f;
	float	 _pad1 = 0.0f;
	float	 _pad2 = 0.0f;
};

bool Mupfel::ECSRenderer::Init(
	nvrhi::DeviceHandle			  device,
	ImageManager&				  img_manager,
	const nvrhi::FramebufferInfo& frameBufferInfo,
	uint32_t					  framesInFlight)
{
	(void)framesInFlight;
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
						  .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(2));

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
	uploadList->close();
	device->executeCommandList(uploadList);

	RebuildDescriptorSet(device, img_manager);

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
	(void)current_command_list;
	(void)context;
}

void Mupfel::ECSRenderer::PostUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	UpdateMVP(current_command_list);

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
	current_command_list->drawIndexed(nvrhi::DrawArguments().setVertexCount(6));
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
			.addItem(nvrhi::BindingSetItem::ConstantBuffer(2, mvpBuffer));

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

bool Mupfel::ECSRenderer::OutOfSync(ImageManager& img_manager) const
{
	return boundGeneration != img_manager.GetGeneration();
}
