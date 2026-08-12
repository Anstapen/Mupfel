#include "GeometryRenderer.h"
#include "Core/Application.h"
#include "Quad.h"
#include <cstdint>

using namespace Mupfel;

struct GeometryInstance
{
	glm::vec2 pos1;
	glm::vec2 pos2;
	glm::vec4 color;
	uint32_t  shape;
	float	  _pad0;
	glm::vec2 inner_edge;
};

static const uint32_t default_geometry_count = 1000;

Mupfel::GeometryRenderer::GeometryRenderer(uint32_t frames_in_flight) : SubRenderer(frames_in_flight) {}

bool Mupfel::GeometryRenderer::Init(const Ping::Device& device, Ping::Format swapChainFormat)
{
	logger = Logger::Create("Geometry Renderer");
	Ping::PipelineSpecification pipeline_spec{
		"Shaders/geo.spv",
		Quad::GetVertexLayout(),
		{{.set = geometrySetIndex,
		  .binding = 0,
		  .type = Ping::DescriptorType::StorageBuffer,
		  .stageFlags = Ping::ShaderStage::Vertex}},
		Ping::CullMode::None,
		Ping::BlendFactor::Zero,
		false,
		swapChainFormat};
	try
	{
		pipeline = device.CreatePipeline(pipeline_spec);
	}
	catch (std::runtime_error err)
	{
		logger->error("Unable to create Pipeline: {}", err.what());
		return false;
	}

	if (!pipeline)
	{
		logger->error("Could not create Pipeline!");
		return false;
	}

	/* We need one vertex buffer for each frame in flight */
	for (uint32_t i = 0; i < framesInFlight; i++)
	{
		/* Create vertex buffers. These hold the 4 vertices used to draw the quad. */
		auto& buffer = vertex_buffers.emplace_back(device.CreateBuffer(
			sizeof(Quad) * quadVertices.size(), Ping::BufferUsage::VertexBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));
		auto* mapped_ptr = static_cast<Quad*>(buffer.GetMappedPtr());

		/* Copy vertices */
		std::memcpy(mapped_ptr, quadVertices.data(), buffer.Size());

		/* Create transform buffers to render entities */
		geometryInstanceBuffers.emplace_back(device.CreateBuffer(
			sizeof(GeometryInstance) * default_geometry_count, Ping::BufferUsage::StorageBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));
	}
	geometryCapacity = default_geometry_count;

	geometryDescriptorSets =
		device.CreateStorageDescriptorSets(pipeline.value(), geometrySetIndex, geometryInstanceBuffers);

	if (!geometryDescriptorSets)
	{
		logger->error("Could not create Descriptor Sets for geometry instances!");
		return false;
	}

	index_buffer = std::move(device.CreateBuffer(
		sizeof(uint16_t) * quadIndices.size(), Ping::BufferUsage::IndexBuffer | Ping::BufferUsage::TransferDst,
		Ping::MemoryProperty::DeviceLocal));

	if (!index_buffer)
	{
		logger->error("Could not create Index Buffers!");
		return false;
	}

	/* Copy indices */
	index_buffer.value().CopyHostData(device, quadIndices.data(), sizeof(uint16_t) * quadIndices.size());

	return true;
}

void Mupfel::GeometryRenderer::PreUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer)
{
	drawable_items = 0;
}

void Mupfel::GeometryRenderer::PostUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer)
{ /* If there are no objects to draw, we can early exit. */

	if (drawable_items == 0)
	{
		IncrementFrameIndex();
		return;
	}

	current_command_buffer.BindPipeline(pipeline.value());

	current_command_buffer.BindDescriptorSet(
		pipeline.value(), geometryDescriptorSets.value(), frameIndex, geometrySetIndex);

	current_command_buffer.BindVertexBuffer(vertex_buffers[frameIndex], 0);

	current_command_buffer.BindIndexBuffer(index_buffer.value());

	current_command_buffer.DrawIndexed(static_cast<uint32_t>(quadIndices.size()), drawable_items);

	IncrementFrameIndex();
}

void Mupfel::GeometryRenderer::Rectangle(
	glm::vec2 pos,
	float	  width,
	float	  height,
	glm::vec4 color,
	uint32_t  thickness)
{
	PushObject({pos.x, pos.y}, {width, height}, color, Shape::RECT, 1.0f, thickness);
}

void Mupfel::GeometryRenderer::Circle(glm::vec2 pos, float radius, glm::vec4 color, uint32_t thickness)
{
	PushObject(
		{pos.x - radius, pos.y - radius}, {radius * 2.0f, radius * 2.0f}, color, Shape::CIRCLE, radius, thickness);
}

void Mupfel::GeometryRenderer::EnsureCapacity(uint32_t required_capacity)
{
	if (required_capacity <= geometryCapacity)
	{
		return;
	}

	uint32_t new_capacity = geometryCapacity;
	while (new_capacity < required_capacity)
	{
		new_capacity *= 2;
	}

	const Ping::Device* device = Application::Get().gpu.get();

	/* Every frame-in-flight buffer is recreated together, so no in-flight submission may still be
	 * reading the old buffers/descriptor sets we're about to destroy. */
	device->WaitForCommands();

	geometryInstanceBuffers.clear();
	for (uint32_t i = 0; i < framesInFlight; i++)
	{
		geometryInstanceBuffers.emplace_back(device->CreateBuffer(
			sizeof(GeometryInstance) * new_capacity, Ping::BufferUsage::StorageBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));
	}

	geometryDescriptorSets =
		device->CreateStorageDescriptorSets(pipeline.value(), geometrySetIndex, geometryInstanceBuffers);

	geometryCapacity = new_capacity;
}

void Mupfel::GeometryRenderer::PushObject(
	glm::vec2 pos1,
	glm::vec2 pos2,
	glm::vec4 color,
	Shape	  shape,
	float	  radius,
	float	  thickness)
{
	EnsureCapacity(drawable_items + 1);

	const float screen_w = static_cast<float>(Application::GetCurrentRenderWidth());
	const float screen_h = static_cast<float>(Application::GetCurrentRenderHeight());

	if (screen_w <= 0.0f || screen_h <= 0.0f)
	{
		return;
	}

	/* The quad spans [-0.5, 0.5] around its centre, so convert the top-left pixel rect
	 * into an NDC centre plus an NDC extent. */
	GeometryInstance g{};
	g.pos1.x = ((pos1.x + pos2.x * 0.5f) / screen_w) * 2.0f - 1.0f;
	g.pos1.y = ((pos1.y + pos2.y * 0.5f) / screen_h) * 2.0f - 1.0f;
	g.pos2.x = (pos2.x / screen_w) * 2.0f;
	g.pos2.y = (pos2.y / screen_h) * 2.0f;
	g.color = color;
	g.shape = static_cast<uint32_t>(shape);

	glm::vec2 inner{0.0f};
	if (thickness > 0.0f)
	{
		inner.x = glm::max(1.0f - thickness / (pos2.x * 0.5f), 0.0f);
		inner.y = glm::max(1.0f - thickness / (pos2.y * 0.5f), 0.0f);
	}
	g.inner_edge = inner;

	static_cast<GeometryInstance*>(geometryInstanceBuffers[frameIndex].GetMappedPtr())[drawable_items] = g;
	++drawable_items;
}