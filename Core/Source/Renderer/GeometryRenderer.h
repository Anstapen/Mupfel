#pragma once
#include "Logger.h"
#include "SubRenderer.h"
#include "glm/glm.hpp"
#include <cstdint>

namespace Mupfel
{
class GeometryRenderer : public SubRenderer
{
public:
	GeometryRenderer(uint32_t frames_in_flight);
	bool Init(const Ping::Device& device, Ping::Format swapChainFormat) final;
	void PreUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer) final;
	void PostUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer) final;

	void Rectangle(glm::vec2 pos, float width, float height, glm::vec4 color, uint32_t thickness = 0);
	void Circle(glm::vec2 pos, float radius, glm::vec4 color, uint32_t thickness = 0);

private:
	enum class Shape : uint32_t
	{
		NONE,
		RECT,
		LINE,
		CIRCLE
	};

private:
	void EnsureCapacity(uint32_t required_capacity);
	void PushObject(glm::vec2 pos1, glm::vec2 pos2, glm::vec4 color, Shape shape, float radius, float thickness);

private:
	static constexpr uint32_t	  geometrySetIndex = 0;
	Logger::SafeLoggerPtr		  logger;
	std::optional<Ping::Pipeline> pipeline;
	/** One host-visible vertex buffer per frame in flight. */
	std::vector<Ping::Buffer> vertex_buffers;
	/** One device-local index buffer */
	std::optional<Ping::Buffer> index_buffer;
	/** One storage buffer per frame in flight for the transform data */
	std::vector<Ping::Buffer> geometryInstanceBuffers;
	uint32_t				  geometryCapacity = 0;
	uint32_t				  drawable_items = 0;

	/** Descriptor sets */
	std::optional<Ping::DescriptorSets> geometryDescriptorSets;
};
} // namespace Mupfel
