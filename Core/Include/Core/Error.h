#pragma once
#include <expected>

namespace Mupfel
{

enum class Error
{
	NO_MEMORY,
	INVALID_PARAMETER,
	FILE_NOT_FOUND,
	FILE_WRONG_FORMAT
};

template <typename T> using Expected = std::expected<T, Error>;

} // namespace Mupfel