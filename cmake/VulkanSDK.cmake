# cmake/VulkanSDK.cmake
#
# Locates the Vulkan SDK from the VULKAN_SDK environment variable and asserts its header version, then
# publishes two interface targets:
#
#   Mupfel::VulkanHeaders  include path only -- for vk-bootstrap, which resolves Vulkan entry points by
#                          dlopening the loader at runtime and deliberately links nothing.
#   Mupfel::Vulkan         the above plus the loader import library, for code whose Vulkan calls are
#                          ordinary prototypes resolved at link time (Core's surface/present path, and
#                          glfwCreateWindowSurface).
#
# find_package(Vulkan) is deliberately not used: it would happily fall back to a distro-packaged loader
# and headers, and the whole point of the checks below is that this build pins itself to the SDK named
# by VULKAN_SDK and refuses anything older than the floor.

if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR
        "VULKAN_SDK not set. Please set this variable to the path of the installed Vulkan SDK.")
endif()

file(TO_CMAKE_PATH "$ENV{VULKAN_SDK}" MUPFEL_VULKAN_SDK)

# Handling the windows vs linux directory naming ("Include" and "Lib" vs "include" and "lib").
if(WIN32)
    set(MUPFEL_VULKAN_INCLUDE_DIR "${MUPFEL_VULKAN_SDK}/Include")
    set(MUPFEL_VULKAN_LIB_DIR     "${MUPFEL_VULKAN_SDK}/Lib")
else()
    set(MUPFEL_VULKAN_INCLUDE_DIR "${MUPFEL_VULKAN_SDK}/include")
    set(MUPFEL_VULKAN_LIB_DIR     "${MUPFEL_VULKAN_SDK}/lib")
endif()

if(WIN32)
    set(MUPFEL_VULKAN_LIBRARY "")
    foreach(name IN ITEMS vulkan-1 vulkan)
        if(EXISTS "${MUPFEL_VULKAN_LIB_DIR}/${name}.lib")
            set(MUPFEL_VULKAN_LIBRARY "${MUPFEL_VULKAN_LIB_DIR}/${name}.lib")
            break()
        endif()
    endforeach()

    if(NOT MUPFEL_VULKAN_LIBRARY)
        message(FATAL_ERROR
            "No Vulkan loader import library in ${MUPFEL_VULKAN_LIB_DIR} (looked for vulkan-1.lib and "
            "vulkan.lib). Is the Vulkan SDK install complete?")
    endif()
else()
    # The LunarG Linux tarball keeps the loader in its own prefix: 1.4.357 ships libvulkan.so only
    # under lib/VulkanLoader/lib (which is also what its setup-env.sh puts on LD_LIBRARY_PATH), and
    # nothing under lib/ itself. Without this probe -lvulkan misses the SDK entirely and either fails
    # or silently links whatever older loader the distro has in /usr/lib. Plain lib/ stays as the
    # fallback for older SDKs and for distro-packaged ones (VULKAN_SDK=/usr).
    if(EXISTS "${MUPFEL_VULKAN_LIB_DIR}/VulkanLoader/lib/libvulkan.so")
        set(MUPFEL_VULKAN_LIB_DIR "${MUPFEL_VULKAN_LIB_DIR}/VulkanLoader/lib")
    endif()
    set(MUPFEL_VULKAN_LIBRARY "${MUPFEL_VULKAN_LIB_DIR}/libvulkan.so")
endif()

# Two vendored libraries impose a floor on the SDK's header version, and both fail late and badly
# without this check:
#   * NVRHI's src/vulkan/vulkan-backend.h has `#if (VK_HEADER_VERSION < 318) #error`, which only fires
#     once the build is already compiling nvrhi_vk.
#   * vk-bootstrap is generated against a specific header version (MUPFEL_VK_BOOTSTRAP_VERSION names
#     it) and references structs and enum values that older headers don't declare, so it fails as a
#     wall of "undeclared identifier" rather than as one legible message.
# The higher of the two is the floor. Keep this number and the vk_bootstrap pin in
# cmake/Dependencies.cmake moving together; the NVRHI half only rises when upstream raises its own
# #error.
set(MUPFEL_VULKAN_HEADER_VERSION_FLOOR 357)

set(_mupfel_vulkan_core_header "${MUPFEL_VULKAN_INCLUDE_DIR}/vulkan/vulkan_core.h")
if(NOT EXISTS "${_mupfel_vulkan_core_header}")
    message(FATAL_ERROR
        "Cannot read ${_mupfel_vulkan_core_header}. Is the Vulkan SDK install at ${MUPFEL_VULKAN_SDK} "
        "complete?")
endif()

# Anchored on whitespace so VK_HEADER_VERSION_COMPLETE, defined a few lines below it in terms of this
# macro, cannot match instead.
file(STRINGS "${_mupfel_vulkan_core_header}" _mupfel_version_line
     REGEX "^#define[ \t]+VK_HEADER_VERSION[ \t]+[0-9]+")

if(NOT _mupfel_version_line)
    message(FATAL_ERROR
        "No VK_HEADER_VERSION found in ${_mupfel_vulkan_core_header}. Unexpected SDK layout.")
endif()

string(REGEX MATCH "[0-9]+$" MUPFEL_VULKAN_HEADER_VERSION "${_mupfel_version_line}")

if(MUPFEL_VULKAN_HEADER_VERSION LESS MUPFEL_VULKAN_HEADER_VERSION_FLOOR)
    message(FATAL_ERROR
        "Vulkan SDK at ${MUPFEL_VULKAN_SDK} is too old: VK_HEADER_VERSION is "
        "${MUPFEL_VULKAN_HEADER_VERSION}, but NVRHI and vk-bootstrap need at least "
        "${MUPFEL_VULKAN_HEADER_VERSION_FLOOR} (SDK 1.4.${MUPFEL_VULKAN_HEADER_VERSION_FLOOR} or newer).")
endif()

message(STATUS "Vulkan SDK: ${MUPFEL_VULKAN_SDK} (VK_HEADER_VERSION ${MUPFEL_VULKAN_HEADER_VERSION})")

# SYSTEM on the interface include directory is what keeps Core's /WX and -Werror from firing inside
# <vulkan/vulkan.hpp>, which is 100k+ lines of generated code.
add_library(Mupfel_VulkanHeaders INTERFACE)
add_library(Mupfel::VulkanHeaders ALIAS Mupfel_VulkanHeaders)
target_include_directories(Mupfel_VulkanHeaders SYSTEM INTERFACE "${MUPFEL_VULKAN_INCLUDE_DIR}")

add_library(Mupfel_Vulkan INTERFACE)
add_library(Mupfel::Vulkan ALIAS Mupfel_Vulkan)
target_link_libraries(Mupfel_Vulkan INTERFACE Mupfel::VulkanHeaders "${MUPFEL_VULKAN_LIBRARY}")
