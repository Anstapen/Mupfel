#pragma once
#include <expected>

namespace Mupfel
{

enum class Error
{
	NO_MEMORY,
	INVALID_PARAMETER,
	FILE_NOT_FOUND,
	FILE_WRONG_FORMAT,
	FILE_ALREADY_LOADED,
	GPU_BUFFER_CREATION_FAILED
};

template <typename T> using Expected = std::expected<T, Error>;

} // namespace Mupfel