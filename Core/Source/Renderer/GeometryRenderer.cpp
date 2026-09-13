#include "GeometryRenderer.h"
#include "Core/Application.h"
#include "Quad.h"
#include <cstdint>

using namespace Mupfel;

struct GeometryInstance
{
	/** NDC centre of the quad. */
	glm::vec2 pos1;
	/** Full extent along the quad's local +x, in NDC. Any rotation is baked in. */
	glm::vec2 axis_u;
	glm::vec4 color;
	/** Full extent along the quad's local +y, in NDC. */
	glm::vec2 axis_v;
	/** Inner outline boundary in local units; {0, 0} means filled. */
	glm::vec2 inner_edge;
	uint32_t  shape;
	float	  _pad0;
	glm::vec2 _pad1;
};

bool Mupfel::GeometryRenderer::Init(
	nvrhi::DeviceHandle			  device,
	ImageManager&				  img_manager,
	const nvrhi::FramebufferInfo& frameBufferInfo,
	uint32_t					  framesInFlight)
{
	(void)device;
	(void)img_manager;
	(void)frameBufferInfo;
	(void)framesInFlight;
	logger = Logger::Create("Geometry Renderer");

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
}

void Mupfel::GeometryRenderer::PostUser(
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

	/* A degenerate line has no direction to orient the quad by, and normalising it
	 * would write NaNs into the instance buffer. */
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
	const float screen_w = static_cast<float>(Application::GetCurrentRenderWidth());
	const float screen_h = static_cast<float>(Application::GetCurrentRenderHeight());

	if (screen_w <= 0.0f || screen_h <= 0.0f)
	{
		return;
	}

	/* Callers work in pixels and bake any rotation into the axis vectors, so the
	 * pixel -> NDC conversion happens per component here, *after* rotating. Converting
	 * first and rotating in NDC would shear the shape by the window's aspect ratio. */
	GeometryInstance g{};
	g.pos1.x = (center.x / screen_w) * 2.0f - 1.0f;
	g.pos1.y = (center.y / screen_h) * 2.0f - 1.0f;
	g.axis_u.x = (axis_u.x / screen_w) * 2.0f;
	g.axis_u.y = (axis_u.y / screen_h) * 2.0f;
	g.axis_v.x = (axis_v.x / screen_w) * 2.0f;
	g.axis_v.y = (axis_v.y / screen_h) * 2.0f;
	g.color = color;
	g.shape = static_cast<uint32_t>(shape);

	/* Outline thickness is in pixels; the shader's local frame spans [-1, 1], so one
	 * local unit is half the quad's pixel extent along that axis. */
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

	/* TODO ensure capacity and push object into GPU buffer. */
}