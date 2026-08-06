#pragma once
#include <string>
#include <cstdint>
#include <cstddef>
#include <typeinfo>
#include <bitset>

namespace Mupfel {
	
    /**
     * @brief This class provides Compile-Time Hashing of
     * Strings.
     */
    class Hash {
    public:
        /**
         * @brief Calculate the FNV-1a 64-bit hash of the given string.
         * @param str The string to be hashed.
         * @return The FNV-1a 64-bit hash of the given string.
         */
		static constexpr uint64_t Compute(std::string_view str) { return fnv1a_64(str); }

	private:
		static constexpr uint64_t fnv1a_64(std::string_view str)
		{
			uint64_t hash = 14695981039346656037ull;
			for (char c : str)
			{
				hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
				hash *= 1099511628211ull;
			}
			return hash;
		}

    };

}