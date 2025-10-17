#pragma once
#include <cstdint>

namespace Mupfel {

	template<typename T>
	struct Coordinate
	{
		T x;
		T y;
		bool operator==(const Coordinate<T>& other) const { return x == other.x && y == other.y; }
	};

}