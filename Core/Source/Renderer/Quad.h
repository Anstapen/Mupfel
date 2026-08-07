#pragma once
#include "Ping/Types.h"
#include <array>
#include <cstdint>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Mupfel
{

struct Quad
{
	/** Unit-quad position in [-0.5, 0.5]. Bound to vertex shader location 0. */
	glm::vec2 pos;
	/** Texture coordinate. Bound to vertex shader location 1. */
	glm::vec2 texCoord;

	static Ping::VertexBinding GetVertexLayout()
	{
		return {
			.binding = 0,
			.stride = sizeof(Quad),
			.inputRate = Ping::VertexInputRate::Vertex,
			.attributes = {
				{0, Ping::VertexFormat::Float32x2, offsetof(Quad, pos)},
				{1, Ping::VertexFormat::Float32x2, offsetof(Quad, texCoord)}}};
	}
};

inline constexpr std::array<Quad, 4> quadVertices = {
	Quad{{-0.5f, -0.5f}, {0.0f, 1.0f}}, Quad{{0.5f, -0.5f}, {1.0f, 1.0f}}, Quad{{0.5f, 0.5f}, {1.0f, 0.0f}},
	Quad{{-0.5f, 0.5f}, {0.0f, 0.0f}}};

inline constexpr std::array<uint16_t, 6> quadIndices = {0, 1, 2, 2, 3, 0};
} // namespace Mupfel