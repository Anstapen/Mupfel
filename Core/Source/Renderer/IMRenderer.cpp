#include "IMRenderer.h"
#include "Core/Application.h"
#include <cassert>
#include "Quad.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

static const uint32_t default_entity_capacity = 1000;

bool Mupfel::IMRenderer::Init(const Ping::Device& device, Ping::Format swapChainFormat)
{
	logger = Logger::Create("Immediate Mode Renderer");
	Ping::PipelineSpecification pipeline_spec{
		"Shaders/im.spv",
		Quad::GetVertexLayout(),
		{
			{.set = samplerSetIndex,
			 .binding = 0,
			 .type = Ping::DescriptorType::CombinedImageSampler,
			 .stageFlags = Ping::ShaderStage::Fragment,
			 .count = max_textures},
			{.set = transformSetIndex,
			 .binding = 0,
			 .type = Ping::DescriptorType::StorageBuffer,
			 .stageFlags = Ping::ShaderStage::Vertex},
		},
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
		textureInstanceBuffers.emplace_back(device.CreateBuffer(
			sizeof(TextureInstance) * default_entity_capacity, Ping::BufferUsage::StorageBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));
	}
	transformCapacity = default_entity_capacity;

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

	auto image = Application::GetCurrentImageManager().Load(device, "Images/default.jpg");

	if (!image)
	{
		logger->error("Unable to load default texture!");
		return false;
	}

	UpdateSamplerDescriptors(device);

	return true;
}

void Mupfel::IMRenderer::PreUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer)
{
	drawable_items = 0;
}

void Mupfel::IMRenderer::PostUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer)
{
	/* If there are no objects to draw, we can early exit. */

	if (drawable_items == 0)
	{
		IncrementFrameIndex();
		return;
	}

	current_command_buffer.BindPipeline(pipeline.value());

	if (samplerDescriptorSets.has_value())
	{
		UpdateSamplerDescriptors(device);
		current_command_buffer.BindDescriptorSet(pipeline.value(), samplerDescriptorSets.value(), 0, samplerSetIndex);
	}

	current_command_buffer.BindDescriptorSet(
		pipeline.value(), transformDescriptorSets.value(), frameIndex, transformSetIndex);

	current_command_buffer.BindVertexBuffer(vertex_buffers[frameIndex], 0);

	current_command_buffer.BindIndexBuffer(index_buffer.value());

	current_command_buffer.DrawIndexed(static_cast<uint32_t>(quadIndices.size()), drawable_items);

	IncrementFrameIndex();
}

uint32_t Mupfel::IMRenderer::Button(float x, float y, float width, float height, const std::string& image_path)
{
	if (!UploadImage(image_path))
	{
		/* Something went wrong uploading the image. */
		return 0;
	}
	auto it = images.find(image_path);
	assert(it->second.size() == 3);

	ImageHandle image_h = it->second[0];

	uint32_t return_value = 0;

	double cursor_x = Application::GetCurrentInputManager().GetCurrentCursorX();
	double cursor_y = Application::GetCurrentInputManager().GetCurrentCursorY();

	/* Check collision with the button. */
	bool hovering = (cursor_x < x + width && cursor_x > x && cursor_y < y + height && cursor_y > y);

	if (hovering)
	{
		/* The button is released. TODO: this search is linear currently! */
		if (Application::GetCurrentInputManager().CheckUserInput(UserInput::LEFT_MOUSE_CLICK))
		{
			image_h = it->second[2];
			return_value = 3;
		}
		else if (Application::GetMouseButton(MouseButton::MOUSE_BUTTON_LEFT) == KeyAction::PRESSED)
		{
			image_h = it->second[2];
			return_value = 2;
		}
		else
		{
			image_h = it->second[1];
			return_value = 1;
		}
	}

	PushObject(x, y, width, height, 0.0f, image_h, 1.0f);

	return return_value;
}

bool Mupfel::IMRenderer::UploadImage(const std::string& image_path)
{
	/* Check if the given texture was already used before. */
	auto it = images.find(image_path);

	if (it == images.end())
	{
		/* Load it from disk. The image needs to be a spritesheet with one row, containing 3 images, in the following
		 * order:
		 * 1. The unhovered button.
		 * 2. The hovered button.
		 * 3. The pressed button.
		 */
		auto image_handles = Application::LoadSpriteSheetImages(image_path, {.rows = 1, .columns = 3});

		if (!image_handles)
		{
			return false;
		}

		images[image_path] = image_handles.value();
	}

	/* We should only get here if the image upload succeeded... */
	assert(images.find(image_path) != images.end());

	return true;
}

void Mupfel::IMRenderer::UpdateSamplerDescriptors(const Ping::Device& device)
{
	const std::vector<Ping::Image>& images = Application::GetCurrentImageManager().GetImages();

	if (images.size() == currentImageCount)
	{
		return;
	}
	currentImageCount = static_cast<uint32_t>(images.size());

	std::vector<std::reference_wrapper<const Ping::Sampler>> sampler_refs(images.size(), samplers.front());

	samplerDescriptorSets = device.CreateTextureArrayDescriptorSet(
		pipeline.value(), samplerSetIndex, max_textures, images, sampler_refs, images.front(), samplers.front());
}

void Mupfel::IMRenderer::EnsureTransformCapacity(uint32_t required_capacity)
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

	const Ping::Device* device = Application::Get().gpu.get();

	/* Every frame-in-flight buffer is recreated together, so no in-flight submission may still be
	 * reading the old buffers/descriptor sets we're about to destroy. */
	device->WaitForCommands();

	textureInstanceBuffers.clear();
	for (uint32_t i = 0; i < framesInFlight; i++)
	{
		textureInstanceBuffers.emplace_back(device->CreateBuffer(
			sizeof(TextureInstance) * new_capacity, Ping::BufferUsage::StorageBuffer,
			Ping::MemoryProperty::HostVisible | Ping::MemoryProperty::HostCoherent |
				Ping::MemoryProperty::DeviceLocal));
	}

	transformDescriptorSets =
		device->CreateStorageDescriptorSets(pipeline.value(), transformSetIndex, textureInstanceBuffers);

	transformCapacity = new_capacity;
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
	EnsureTransformCapacity(drawable_items + 1);

	const float screen_w = static_cast<float>(Application::GetCurrentRenderWidth());
	const float screen_h = static_cast<float>(Application::GetCurrentRenderHeight());

	if (screen_w <= 0.0f || screen_h <= 0.0f)
	{
		return;
	}

	/* The quad spans [-0.5, 0.5] around its centre, so convert the top-left pixel rect
	 * into an NDC centre plus an NDC extent. Framebuffer Y and GLFW cursor Y both point
	 * down, so no flip is needed. */
	TextureInstance t{};
	t.pos_x = ((x + width * 0.5f) / screen_w) * 2.0f - 1.0f;
	t.pos_y = ((y + height * 0.5f) / screen_h) * 2.0f - 1.0f;
	t.width = (width / screen_w) * 2.0f;
	t.height = -(height / screen_h) * 2.0f;
	t.rotation = rotation;
	t.index = index;
	t.uvScale = uv_scale;

	static_cast<TextureInstance*>(textureInstanceBuffers[frameIndex].GetMappedPtr())[drawable_items] = t;
	++drawable_items;
}
