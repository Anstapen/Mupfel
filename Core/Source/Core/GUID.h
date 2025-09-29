#pragma once
#include <string>
#include <cstdint>
#include <cstddef>
#include <typeinfo>

namespace Mupfel {
	

    // FNV-1a 64-bit
    constexpr uint64_t fnv1a_64(const char* str, std::size_t n) {
        uint64_t hash = 14695981039346656037ull; // FNV offset basis
        for (std::size_t i = 0; i < n; ++i) {
            hash ^= static_cast<uint64_t>(str[i]);
            hash *= 1099511628211ull; // FNV prime
        }
        return hash;
    }

    // Hilfsfunktion für nullterminierte Strings
    constexpr std::size_t strlen_c(const char* str) {
        std::size_t len = 0;
        while (str[len] != '\0') {
            ++len;
        }
        return len;
    }

    constexpr uint64_t hash_str(const char* str) {
        return fnv1a_64(str, strlen_c(str));
    }

#if 0
    class GUID {
    public:
        template <typename T>
        static constexpr uint64_t Get() {
            return hash_str(typeid(T).name());
        }
    };
#endif


}