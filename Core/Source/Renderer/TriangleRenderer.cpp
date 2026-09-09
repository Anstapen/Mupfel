#include "TriangleRenderer.h"

#include "Shaders/default_fragment.h"
#include "Shaders/default_vertex.h"

struct Vertex
{
	float position[2];
	float color[3];
};

static const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
	{{0.0f, 0.5f}, {1.0f, 1.0f, 1.0f}},
	{{0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}};

bool Mupfel::TriangleRenderer::Init(
	nvrhi::DeviceHandle			  device,
	const nvrhi::FramebufferInfo& frameBufferInfo,
	uint32_t					  framesInFlight)
{
	(void)framesInFlight;
	nvrhi::ShaderHandle vertexShader = device->createShader(
		nvrhi::ShaderDesc().setShaderType(nvrhi::ShaderType::Vertex).setEntryName("vertMain"), defaultVertex,
		defaultVertex_sizeInBytes);

	nvrhi::VertexAttributeDesc attributes[] = {
		nvrhi::VertexAttributeDesc()
			.setName("POSITION")
			.setFormat(nvrhi::Format::RG32_FLOAT)
			.setOffset(offsetof(Vertex, position))
			.setElementStride(sizeof(Vertex)),
		nvrhi::VertexAttributeDesc()
			.setName("COLOR")
			.setFormat(nvrhi::Format::RGB32_FLOAT)
			.setOffset(offsetof(Vertex, color))
			.setElementStride(sizeof(Vertex))};

	nvrhi::InputLayoutHandle inputLayout =
		device->createInputLayout(attributes, uint32_t(std::size(attributes)), vertexShader);

	nvrhi::ShaderHandle fragmentShader = device->createShader(
		nvrhi::ShaderDesc().setShaderType(nvrhi::ShaderType::Pixel).setEntryName("fragMain"), defaultFragment,
		defaultFragment_sizeInBytes);

	auto pipelineDesc =
		nvrhi::GraphicsPipelineDesc()
			.setInputLayout(inputLayout)
			.setVertexShader(vertexShader)
			.setPixelShader(fragmentShader)
			.setRenderState(
				nvrhi::RenderState()
					.setDepthStencilState(nvrhi::DepthStencilState().disableDepthTest().disableDepthWrite())
					.setRasterState(nvrhi::RasterState().setCullMode(nvrhi::RasterCullMode::None)));

	this->pipeline = device->createGraphicsPipeline(pipelineDesc, frameBufferInfo);

	auto vertexBufferDesc = nvrhi::BufferDesc()
								.setByteSize(vertices.size() * sizeof(Vertex))
								.setIsVertexBuffer(true)
								.enableAutomaticStateTracking(nvrhi::ResourceStates::VertexBuffer)
								.setDebugName("Vertex Buffer");
	this->vertexBuffer = device->createBuffer(vertexBufferDesc);

	/* Upload the buffer to the GPU. */
	nvrhi::CommandListHandle uploadList = device->createCommandList();
	uploadList->open();
	uploadList->writeBuffer(vertexBuffer, vertices.data(), vertices.size() * sizeof(Vertex));
	uploadList->close();
	device->executeCommandList(uploadList);

	return true;
}

void Mupfel::TriangleRenderer::PreUser(
	nvrhi::DeviceHandle		 device,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;

	auto state = nvrhi::GraphicsState()
					 .setPipeline(pipeline)
					 .setFramebuffer(context.frameBuffer)
					 .setViewport(
						 nvrhi::ViewportState().addViewportAndScissorRect(
							 nvrhi::Viewport(float(context.width), float(context.height))))
					 .addVertexBuffer(nvrhi::VertexBufferBinding().setBuffer(vertexBuffer).setSlot(0).setOffset(0));

	current_command_list->setGraphicsState(state);
	current_command_list->draw(nvrhi::DrawArguments().setVertexCount(3));
}

void Mupfel::TriangleRenderer::PostUser(
	nvrhi::DeviceHandle		 device,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)current_command_list;
	(void)context;
}
