#pragma once
#include "Ping/Buffer.h"
#include "Ping/Pipeline.h"
#include "Ping/Sampler.h"
#include "SubRenderer.h"
#include "Logger.h"
#include <optional>

namespace Mupfel
{
class ECSRenderer : public SubRenderer
{
public:
	ECSRenderer(uint32_t frames_in_flight);
public:
	bool Init(const Ping::Device& device, Ping::Format swapChainFormat) final;
	void PreUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer) final;
	void PostUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer) final;

private:
	void EnsureTransformCapacity(const Ping::Device& device, uint32_t required_capacity);
	void EnsureLightCapacity(const Ping::Device& device, uint32_t required_capacity);
	void SyncRenderableObjects(const Ping::Device& device, uint32_t frame_index);
	void SyncLights(const Ping::Device& device, uint32_t frame_index);
	void UpdateMVP(Ping::Buffer& uniform_buffer);
	void UpdateSamplerDescriptors(const Ping::Device& device);

private:
	static constexpr uint32_t uboSetIndex = 0;
	static constexpr uint32_t samplerSetIndex = 1;
	static constexpr uint32_t transformSetIndex = 2;
	static constexpr uint32_t lightSetIndex = 3;
	static constexpr uint32_t lightParamSetIndex = 4;
	static constexpr uint32_t max_textures = 4096;

	Logger::SafeLoggerPtr logger;

	std::optional<Ping::Pipeline> pipeline;

	/** One host-visible vertex buffer per frame in flight. */
	std::vector<Ping::Buffer> vertex_buffers;
	/** One device-local index buffer */
	std::optional<Ping::Buffer> index_buffer;
	/** One uniform buffer per frame in flight */
	std::vector<Ping::Buffer> uniformBuffers;
	/** One storage buffer per frame in flight for the transform data */
	std::vector<Ping::Buffer> textureInstanceBuffers;
	std::vector<Ping::Buffer> lightInstanceBuffers;
	/** One uniform buffer per frame in flight */
	std::vector<Ping::Buffer> lightParamBuffers;
	uint32_t				  transformCapacity = 0;
	uint32_t				  lightCapacity = 0;
	uint32_t				  drawable_entities = 0;
	/** Descriptor sets */
	std::optional<Ping::DescriptorSets> descriptorSets;
	std::optional<Ping::DescriptorSets> transformDescriptorSets;
	std::optional<Ping::DescriptorSets> lightDescriptorSets;
	std::optional<Ping::DescriptorSets> lightParamDescriptorSets;
	std::vector<Ping::Sampler>			samplers;
	std::optional<Ping::DescriptorSets> samplerDescriptorSets;
	uint32_t							currentImageCount = 0;

};

} // namespace Mupfel
