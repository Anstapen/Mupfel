#include "GeometryRenderer.h"
#include "Core/Application.h"
#include "Quad.h"
#include <cstdint>

#include "Shaders/geo_fragment.h"
#include "Shaders/geo_vertex.h"

using namespace Mupfel;

bool Mupfel::GeometryRenderer::Init(
	nvrhi::DeviceHandle			  device,
	ImageManager&				  img_manager,
	const nvrhi::FramebufferInfo& frameBufferInfo,
	uint32_t					  framesInFlight)
{
	(void)img_manager;
	(void)framesInFlight;
	logger = Logger::Create("Geometry Renderer");

	nvrhi::ShaderHandle vertexShader = device->createShader(
		nvrhi::ShaderDesc().setShaderType(nvrhi::ShaderType::Vertex).setEntryName("vertMain"), geoVertex,
		geoVertex_sizeInBytes);

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
		nvrhi::ShaderDesc().setShaderType(nvrhi::ShaderType::Pixel).setEntryName("fragMain"), geoFragment,
		geoFragment_sizeInBytes);

	auto layoutDesc = nvrhi::BindingLayoutDesc()
						  .setVisibility(nvrhi::ShaderType::All)
						  .setBindingOffsets(
							  nvrhi::VulkanBindingOffsets()
								  .setShaderResourceOffset(0)
								  .setSamplerOffset(0)
								  .setConstantBufferOffset(0)
								  .setUnorderedAccessViewOffset(0))
						  .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0));

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

	return true;
}

void Mupfel::GeometryRenderer::PreUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)img_manager;
	(void)current_command_list;
	(void)context;

	geometryBuffer.Clear();
}

void Mupfel::GeometryRenderer::PostUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)img_manager;

	if (geometryBuffer.Empty())
	{
		return;
	}

	uint32_t drawableItems =
		static_cast<uint32_t>(std::min<uint64_t>(geometryBuffer.Size(), std::numeric_limits<uint32_t>::max()));

	DualBufferStatus status = geometryBuffer.FlushToGPU(device, current_command_list);

	if (status == DualBufferStatus::BUFFER_ALLOCATION_FAILED)
	{
		return;
	}

	if (status == DualBufferStatus::BUFFER_RESIZED)
	{
		RebuildDescriptorSet(device);
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

void Mupfel::GeometryRenderer::Rectangle(glm::vec2 pos, float width, float height, glm::vec4 color, uint32_t thickness)
{
	PushObject(
		{pos.x + width * 0.5f, pos.y + height * 0.5f}, {width, 0.0f}, {0.0f, height}, color, Shape::RECT,
		static_cast<float>(thickness));
}

void Mupfel::GeometryRenderer::Circle(glm::vec2 pos, float radius, glm::vec4 color, uint32_t thickness)
{
	PushObject(
		{pos.x, pos.y}, {radius * 2.0f, 0.0f}, {0.0f, radius * 2.0f}, color, Shape::CIRCLE,
		static_cast<float>(thickness));
}

void Mupfel::GeometryRenderer::Line(glm::vec2 start, glm::vec2 end, glm::vec4 color)
{
	const glm::vec2 delta = end - start;
	const float		length = glm::length(delta);

	if (length < 1e-6f)
	{
		return;
	}

	const glm::vec2 dir = delta / length;
	const glm::vec2 normal{-dir.y, dir.x};

	PushObject((start + end) * 0.5f, dir * length, normal * glm::vec2(2.0f), color, Shape::LINE, 0.0f);
}

void Mupfel::GeometryRenderer::PushObject(
	glm::vec2 center,
	glm::vec2 axis_u,
	glm::vec2 axis_v,
	glm::vec4 color,
	Shape	  shape,
	float	  thickness)
{
	if (Application::IsWindowMinimized())
	{
		return;
	}

	const float screen_w = static_cast<float>(Application::GetCurrentRenderWidth());
	const float screen_h = static_cast<float>(Application::GetCurrentRenderHeight());

	if (screen_w <= 0.0f || screen_h <= 0.0f)
	{
		return;
	}

	GeometryInstance g{};
	g.pos1.x = (center.x / screen_w) * 2.0f - 1.0f;
	g.pos1.y = (center.y / screen_h) * 2.0f - 1.0f;
	g.axis_u.x = (axis_u.x / screen_w) * 2.0f;
	g.axis_u.y = (axis_u.y / screen_h) * 2.0f;
	g.axis_v.x = (axis_v.x / screen_w) * 2.0f;
	g.axis_v.y = (axis_v.y / screen_h) * 2.0f;
	g.color = color;
	g.shape = static_cast<uint32_t>(shape);

	g.inner_edge = glm::vec2(0.0f);
	if (thickness > 0.0f)
	{
		const float half_u = glm::length(axis_u) * 0.5f;
		const float half_v = glm::length(axis_v) * 0.5f;
		if (half_u > 0.0f && half_v > 0.0f)
		{
			g.inner_edge.x = glm::max(1.0f - thickness / half_u, 0.0f);
			g.inner_edge.y = glm::max(1.0f - thickness / half_v, 0.0f);
		}
	}

	geometryBuffer.PushBack(g);
}

void Mupfel::GeometryRenderer::RebuildDescriptorSet(nvrhi::DeviceHandle device)
{
	nvrhi::BindingSetDesc bindingSetDesc = nvrhi::BindingSetDesc().addItem(
		nvrhi::BindingSetItem::StructuredBuffer_SRV(0, geometryBuffer.GetGPUBufferHandle()));

	bindingSet = device->createBindingSet(bindingSetDesc, bindingLayout);
}
