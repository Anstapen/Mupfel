#include "ECSRenderer.h"
#include "Core/Application.h"
#include "ImageManager.h"

#include "ECS/Components/Animation.h"
#include "ECS/Components/Light.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Transform.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct UniformBufferObject
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 cameraRight;
	glm::vec4 cameraPos;
};

struct Vertex
{
	/** 2D position, bound to vertex shader location 0. */
	glm::vec2 pos;
	/** RGB color, bound to vertex shader location 1. */
	glm::vec2 texCoord;

	/** The `Ping::VertexBinding` matching `Transform`'s memory layout, for `PipelineSpecification::vertexLayout`. */
	static Ping::VertexBinding GetVertexLayout()
	{
		return {
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = Ping::VertexInputRate::Vertex,
			.attributes = {
				{0, Ping::VertexFormat::Float32x2, offsetof(Vertex, pos)},
				{1, Ping::VertexFormat::Float32x2, offsetof(Vertex, texCoord)}}};
	}
};

static const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {0.0f, 1.0f}},
	{{0.5f, -0.5f}, {1.0f, 1.0f}},
	{{0.5f, 0.5f}, {1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f}}};

static const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

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
	float	 uvScale = 1.0f;
	uint32_t emitsLight = 0;
	uint32_t frame = 0;
	float	 _pad1;
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

static const uint32_t default_entity_capacity = 100000;
static const uint32_t default_light_capacity = 100;

/* TODO: encapsulate this in separate Camera class! */
//static float	 cameraYaw = glm::radians(-135.0f);
//static float	 cameraPitch = glm::radians(30.0f);

Mupfel::ECSRenderer::ECSRenderer(uint32_t frames_in_flight) : SubRenderer(frames_in_flight) {}

bool Mupfel::ECSRenderer::Init(const Ping::Device& device, Ping::Format swapChainFormat)
{
	logger = Logger::Create("ECS Renderer");
	Ping::PipelineSpecification pipeline_spec{
		"Shaders/ecs.spv",
		Vertex::GetVertexLayout(),
		{{.set = uboSetIndex,
		  .binding = 0,
		  .type = Ping::DescriptorType::UniformBuffer,
		  .stageFlags = Ping::ShaderStage::Vertex | Ping::ShaderStage::Fragment},
		 {.set = samplerSetIndex,
		  .binding = 0,
		  .type = Ping::DescriptorType::CombinedImageSampler,
		  .stageFlags = Ping::ShaderStage::Fragment,
		  .count = max_textures},
		 {.set = transformSetIndex,
		  .binding = 0,
		  .type = Ping::DescriptorType::StorageBuffer,
		  .stageFlags = Ping::ShaderStage::Vertex},
		 {.set = lightSetIndex,
		  .binding = 0,
		  .type = Ping::DescriptorType::StorageBuffer,
		  .stageFlags = Ping::ShaderStage::Fragment},
		 {.set = lightParamSetIndex,
		  .binding = 0,
		  .type = Ping::DescriptorType::UniformBuffer,
		  .stageFlags = Ping::ShaderStage::Fragment}},
		Ping::CullMode::Back,
		Ping::BlendFactor::Zero,
		true,
		swapChainFormat};
	pipeline = device.CreatePipeline(pipeline_spec);

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
			sizeof(Vertex) * vertices.size(), Ping::BufferUsage::VertexBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));
		auto* mapped_ptr = static_cast<Vertex*>(buffer.GetMappedPtr());

		/* Copy vertices */
		std::memcpy(mapped_ptr, vertices.data(), buffer.Size());

		/* Create uniform buffers for the MVP matrices */
		uniformBuffers.emplace_back(device.CreateBuffer(
			sizeof(UniformBufferObject), Ping::BufferUsage::UniformBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));

		/* Create transform buffers to render entities */
		textureInstanceBuffers.emplace_back(device.CreateBuffer(
			sizeof(TextureInstance) * default_entity_capacity, Ping::BufferUsage::StorageBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));

		lightInstanceBuffers.emplace_back(device.CreateBuffer(
			sizeof(LightInstance) * default_light_capacity, Ping::BufferUsage::StorageBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));

		lightParamBuffers.emplace_back(device.CreateBuffer(
			sizeof(LightParams), Ping::BufferUsage::UniformBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));
	}
	transformCapacity = default_entity_capacity;

	index_buffer = std::move(device.CreateBuffer(
		sizeof(uint16_t) * indices.size(), Ping::BufferUsage::IndexBuffer | Ping::BufferUsage::TransferDst,
		Ping::MemoryProperty::DeviceLocal));

	if (!index_buffer)
	{
		logger->error("Could not create Index Buffers!");
		return false;
	}

	/* Copy indices */
	index_buffer.value().CopyHostData(device, indices.data(), sizeof(uint16_t) * indices.size());

	/* Create descriptor sets */
	descriptorSets = device.CreateDescriptorSets(pipeline.value(), uboSetIndex, uniformBuffers);

	if (!descriptorSets)
	{
		logger->error("Could not create Descriptor Sets for UBOs!");
		return false;
	}

	lightParamDescriptorSets = device.CreateDescriptorSets(pipeline.value(), lightParamSetIndex, lightParamBuffers);

	if (!lightParamDescriptorSets)
	{
		logger->error("Could not create Descriptor Sets for Light parameters!");
		return false;
	}

	samplers.push_back(device.CreateSampler(
		{.filterMode = Ping::SamplerFilterMode::Nearest,
		 .mipmapMode = Ping::SamplerMipMapMode::Nearest,
		 .addressMode = Ping::SamplerAddressMode::ClampToEdge,
		 .anisotropyEnable = false}));

	transformDescriptorSets =
		device.CreateStorageDescriptorSets(pipeline.value(), transformSetIndex, textureInstanceBuffers);

	if (!transformDescriptorSets)
	{
		logger->error("Could not create Descriptor Sets for Texture instances!");
		return false;
	}

	lightDescriptorSets = device.CreateStorageDescriptorSets(pipeline.value(), lightSetIndex, lightInstanceBuffers);

	if (!lightDescriptorSets)
	{
		logger->error("Could not create Descriptor Sets for Light instances!");
		return false;
	}

	lightCapacity = default_light_capacity;

	auto image = Application::GetCurrentImageManager().Load(device, "Images/default.jpg");

	if (!image)
	{
		logger->error("Unable to load default texture!");
		return false;
	}


	UpdateSamplerDescriptors(device);

	return true;
}

void Mupfel::ECSRenderer::PreUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer)
{ /* Currently the ECS renderer does all the work after user input. */ }

void Mupfel::ECSRenderer::PostUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer)
{
	UpdateMVP(uniformBuffers[frameIndex]);
	SyncRenderableObjects(device, frameIndex);
	SyncLights(device, frameIndex);

	current_command_buffer.BindPipeline(pipeline.value());

	current_command_buffer.BindDescriptorSet(pipeline.value(), descriptorSets.value(), frameIndex, uboSetIndex);
	current_command_buffer.BindDescriptorSet(
		pipeline.value(), lightParamDescriptorSets.value(), frameIndex, lightParamSetIndex);

	if (samplerDescriptorSets.has_value())
	{
		UpdateSamplerDescriptors(device);
		current_command_buffer.BindDescriptorSet(pipeline.value(), samplerDescriptorSets.value(), 0, samplerSetIndex);
	}

	current_command_buffer.BindDescriptorSet(
		pipeline.value(), transformDescriptorSets.value(), frameIndex, transformSetIndex);

	current_command_buffer.BindDescriptorSet(pipeline.value(), lightDescriptorSets.value(), frameIndex, lightSetIndex);

	current_command_buffer.BindVertexBuffer(vertex_buffers[frameIndex], 0);

	current_command_buffer.BindIndexBuffer(index_buffer.value());

	// current_command_buffer.Draw(3);
	current_command_buffer.DrawIndexed(static_cast<uint32_t>(indices.size()), drawable_entities);

	IncrementFrameIndex();
}

void Mupfel::ECSRenderer::EnsureTransformCapacity(const Ping::Device& device, uint32_t required_capacity)
{
	if (required_capacity <= transformCapacity)
	{
		return;
	}

	uint32_t new_capacity = transformCapacity;
	while (new_capacity < required_capacity)
	{
		new_capacity *= 2;
	}

	/* Every frame-in-flight buffer is recreated together, so no in-flight submission may still be
	 * reading the old buffers/descriptor sets we're about to destroy. */
	device.WaitForCommands();

	textureInstanceBuffers.clear();
	for (uint32_t i = 0; i < framesInFlight; i++)
	{
		textureInstanceBuffers.emplace_back(device.CreateBuffer(
			sizeof(TextureInstance) * new_capacity, Ping::BufferUsage::StorageBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));
	}

	transformDescriptorSets =
		device.CreateStorageDescriptorSets(pipeline.value(), transformSetIndex, textureInstanceBuffers);

	transformCapacity = new_capacity;
}

void Mupfel::ECSRenderer::EnsureLightCapacity(const Ping::Device& device, uint32_t required_capacity)
{
	if (required_capacity <= lightCapacity)
	{
		return;
	}

	uint32_t new_capacity = lightCapacity;
	while (new_capacity < required_capacity)
	{
		new_capacity *= 2;
	}

	/* Every frame-in-flight buffer is recreated together, so no in-flight submission may still be
	 * reading the old buffers/descriptor sets we're about to destroy. */
	device.WaitForCommands();

	lightInstanceBuffers.clear();
	for (uint32_t i = 0; i < framesInFlight; i++)
	{
		lightInstanceBuffers.emplace_back(device.CreateBuffer(
			sizeof(LightInstance) * new_capacity, Ping::BufferUsage::StorageBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));
	}

	lightDescriptorSets = device.CreateStorageDescriptorSets(pipeline.value(), lightSetIndex, lightInstanceBuffers);

	lightCapacity = new_capacity;
}

void Mupfel::ECSRenderer::SyncRenderableObjects(const Ping::Device& device, uint32_t frame_index)
{
	Mupfel::Registry& registry = Mupfel::Application::GetCurrentRegistry();
	EnsureTransformCapacity(device, registry.GetCurrentEntities());

	uint32_t buffer_index = 0;

	TextureInstance* buffer = static_cast<TextureInstance*>(textureInstanceBuffers[frame_index].GetMappedPtr());

	for (auto [e, texture, transform] : registry.view<Mupfel::Texture, Mupfel::Transform>())
	{
		buffer[buffer_index].index = texture.index;
		buffer[buffer_index].uvScale = texture.uvScale;
		buffer[buffer_index].pos_x = transform.pos_x;
		buffer[buffer_index].pos_y = transform.pos_y;
		buffer[buffer_index].pos_z = transform.pos_z;
		buffer[buffer_index].rotation = transform.rotation;
		buffer[buffer_index].scale_x = transform.scale_x;
		buffer[buffer_index].scale_y = transform.scale_y;

		/* TODO: check if GetSignature might be more performant! */
		if (registry.HasComponent<Mupfel::Light>(e))
		{
			buffer[buffer_index].emitsLight = 1;
		}

		if (registry.HasComponent<Mupfel::Animation>(e))
		{
			buffer[buffer_index].frame = registry.GetComponent<Mupfel::Animation>(e).currentFrame;
		}
		else
		{
			buffer[buffer_index].frame = 0; // static: layer 0
		}

		buffer_index++;
	}

	drawable_entities = buffer_index;
}

void Mupfel::ECSRenderer::SyncLights(const Ping::Device& device, uint32_t frame_index)
{
	Mupfel::Registry& registry = Mupfel::Application::GetCurrentRegistry();
	uint32_t		  buffer_index = 0;

	for (auto [e, light, transform] : registry.view<Mupfel::Light, Mupfel::Transform>())
	{

		EnsureLightCapacity(device, buffer_index + 1);
		LightInstance* buffer = static_cast<LightInstance*>(lightInstanceBuffers[frame_index].GetMappedPtr());
		buffer[buffer_index].ambientStrength = light.ambientStrength;

		/* Populate the LightInstance object for the fragment shader */
		buffer[buffer_index].ambientStrength = light.ambientStrength;
		buffer[buffer_index].colorRGB.r = light.r;
		buffer[buffer_index].colorRGB.g = light.g;
		buffer[buffer_index].colorRGB.b = light.b;
		buffer[buffer_index].pos.x = transform.pos_x;
		buffer[buffer_index].pos.y = transform.pos_y;
		buffer[buffer_index].pos.z = transform.pos_z;

		buffer_index++;
	}

	LightParams params{};
	params.numLights = buffer_index;
	std::memcpy(lightParamBuffers[frame_index].GetMappedPtr(), &params, sizeof(LightParams));
}

void Mupfel::ECSRenderer::UpdateMVP(Ping::Buffer& uniform_buffer)
{
	int32_t width = Application::GetCurrentRenderWidth();
	int32_t height = Application::GetCurrentRenderHeight();
	Camera	cam = Application::GetCurrentSceneCamera();

	glm::vec3 cameraTarget = glm::vec3(cam.target_x, cam.target_y, cam.target_z);


	glm::vec3 eye =
		cameraTarget + cam.distance * glm::vec3(
											glm::cos(cam.pitch) * glm::cos(cam.yaw),
														glm::cos(cam.pitch) * glm::sin(cam.yaw), glm::sin(cam.pitch));

	UniformBufferObject ubo{};
	ubo.view = lookAt(eye, cameraTarget, glm::vec3(0.0f, 0.0f, 1.0f));
	const float aspect = static_cast<float>(width) / static_cast<float>(height);
	const float half_height = cam.distance * glm::tan(glm::radians(45.0f) * 0.5f);
	const float half_width = half_height * aspect;

	ubo.proj = glm::ortho(-half_width, half_width, -half_height, half_height, 0.1f, 500.0f);

	ubo.proj[1][1] *= -1;
	ubo.cameraRight = glm::vec4(-glm::sin(cam.yaw), glm::cos(cam.yaw), 0.0f, 0.0f);
	ubo.cameraPos = glm::vec4(eye, 1.0f);
	std::memcpy(uniform_buffer.GetMappedPtr(), &ubo, sizeof(UniformBufferObject));
}

void Mupfel::ECSRenderer::UpdateSamplerDescriptors(const Ping::Device& device)
{
	const std::vector<Ping::Image>&							 images = Application::GetCurrentImageManager().GetImages();

	if (images.size() == currentImageCount)
	{
		return;
	}
	currentImageCount = static_cast<uint32_t>(images.size());

	std::vector<std::reference_wrapper<const Ping::Sampler>> sampler_refs(images.size(), samplers.front());

	samplerDescriptorSets = device.CreateTextureArrayDescriptorSet(
		pipeline.value(), samplerSetIndex, max_textures, images, sampler_refs, images.front(),
		samplers.front());
}
