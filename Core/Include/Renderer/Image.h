#pragma once
#include <cstdint>

namespace Mupfel
{

typedef uint32_t ImageHandle;

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
