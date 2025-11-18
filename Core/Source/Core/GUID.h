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
        static constexpr uint64_t Compute(const char* str) {
            return fnv1a_64(str, strlen_c(str));
        }

        static constexpr uint64_t Compute_n(const char* str, std::size_t size) {
            return fnv1a_64(str, size);
        }

    private:
        /**
         * @brief Compute the FNV-1a 64-bit Hash for the given string at compile time.
         * @param str The string to be hashed.
         * @param n The length of the given string.
         * @return The FNV-1a 64-bit hash of the given string.
         */
        static constexpr uint64_t fnv1a_64(const char* str, std::size_t n) {
            uint64_t hash = 14695981039346656037ull; // FNV offset basis
            for (std::size_t i = 0; i < n; ++i) {
                hash ^= static_cast<uint64_t>(str[i]);
                hash *= 1099511628211ull; // FNV prime
            }
            return hash;
        }

        /**
         * @brief Caclulate the length of the given string at compile time.
         * @param str The string.
         * @return The length of the given string.
         */
        static constexpr std::size_t strlen_c(const char* str) {
            std::size_t len = 0;
            while (str[len] != '\0') {
                ++len;
            }
            return len;
        }

        

        

    };

}