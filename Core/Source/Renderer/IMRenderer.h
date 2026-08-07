#pragma once
#include "ImageManager.h"
#include "Logger.h"
#include "SubRenderer.h"
#include <string>
#include <unordered_map>
#include <vector>

#include "Ping/Pipeline.h"

namespace Mupfel
{
class IMRenderer : public SubRenderer
{
public:
	IMRenderer(uint32_t frames_in_flight) : SubRenderer(frames_in_flight) {}

	bool Init(const Ping::Device& device, Ping::Format swapChainFormat) final;
	void PreUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer) final;
	void PostUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer) final;

	/**
	 * Display a button with the given texture.
	 *
	 * \param x x-offset in screen space.
	 * \param y y-offset in screen space.
	 * \param width The width of the button. This is independent of the used texture.
	 * \param height The height of the button. This is independent of the used texture.
	 * \param image_path Path to an image containing the button texture.
	 * \return 0 if the cursor is not overlapping the button, 1 if the cursor is hovering over the button, 2 if the
	 * button is pressed.
	 */
	uint32_t Button(float x, float y, float width, float height, const std::string& image_path);

private:
	bool UploadImage(const std::string& image_path);
	void UpdateSamplerDescriptors(const Ping::Device& device);
	void EnsureTransformCapacity(uint32_t required_capacity);
	void PushObject(
		float				x,
		float				y,
		float				width,
		float				height,
		float				rotation,
		uint32_t			index,
		float				uv_scale);

private:
	static constexpr uint32_t								  samplerSetIndex = 0;
	static constexpr uint32_t								  transformSetIndex = 1;
	static constexpr uint32_t								  max_textures = 4096;
	Logger::SafeLoggerPtr									  logger;
	std::unordered_map<std::string, std::vector<ImageHandle>> images;
	std::optional<Ping::Pipeline>							  pipeline;
	/** One host-visible vertex buffer per frame in flight. */
	std::vector<Ping::Buffer> vertex_buffers;
	/** One device-local index buffer */
	std::optional<Ping::Buffer> index_buffer;
	/** One storage buffer per frame in flight for the transform data */
	std::vector<Ping::Buffer> textureInstanceBuffers;
	uint32_t				  transformCapacity = 0;
	uint32_t				  drawable_items = 0;

	/** Descriptor sets */
	std::optional<Ping::DescriptorSets> transformDescriptorSets;
	std::optional<Ping::DescriptorSets> lightDescriptorSets;
	std::optional<Ping::DescriptorSets> lightParamDescriptorSets;
	std::vector<Ping::Sampler>			samplers;
	std::optional<Ping::DescriptorSets> samplerDescriptorSets;
	uint32_t							currentImageCount = 0;
};
} // namespace Mupfel
