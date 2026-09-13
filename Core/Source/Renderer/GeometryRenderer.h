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
	bool Init(
		nvrhi::DeviceHandle			  device,
		ImageManager&				  img_manager,
		const nvrhi::FramebufferInfo& frameBufferInfo,
		uint32_t					  framesInFlight) final;
	void PreUser(
		nvrhi::DeviceHandle		 device,
		ImageManager&			 img_manager,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context)
		final;
	void PostUser(
		nvrhi::DeviceHandle		 device,
		ImageManager&			 img_manager,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context) final;

	void Rectangle(glm::vec2 pos, float width, float height, glm::vec4 color, uint32_t thickness = 0);
	void Circle(glm::vec2 pos, float radius, glm::vec4 color, uint32_t thickness = 0);
	void Line(glm::vec2 start, glm::vec2 end, glm::vec4 color);

private:
	enum class Shape : uint32_t
	{
		NONE,
		RECT,
		LINE,
		CIRCLE
	};

private:
	void
	PushObject(glm::vec2 center, glm::vec2 axis_u, glm::vec2 axis_v, glm::vec4 color, Shape shape, float thickness);

private:
	Logger::SafeLoggerPtr logger;
};
} // namespace Mupfel
