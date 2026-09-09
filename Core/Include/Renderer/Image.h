#pragma once
#include <cstdint>

namespace Mupfel
{

typedef uint32_t ImageHandle;

/** Handle returned when a load fails or a slot was never filled. */
inline constexpr ImageHandle INVALID_IMAGE = 0;

/** Upper bound on concurrently resident images. */
inline constexpr uint32_t MAX_IMAGE_COUNT = 8192;

/**
 * This structure is used to describe an animated image.
 * The engine expects animated images in row-major format.
 */
struct ImageSpecification
{
	/** The number of rows in the image. */
	uint32_t rows;
	/** The number of columns in the image. */
	uint32_t columns;
};

} // namespace Mupfel
