if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR
        "VULKAN_SDK not set. Please set this variable to the path of the installed Vulkan SDK.")
endif()

file(TO_CMAKE_PATH "$ENV{VULKAN_SDK}" MUPFEL_VULKAN_SDK)

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
    if(EXISTS "${MUPFEL_VULKAN_LIB_DIR}/VulkanLoader/lib/libvulkan.so")
        set(MUPFEL_VULKAN_LIB_DIR "${MUPFEL_VULKAN_LIB_DIR}/VulkanLoader/lib")
    endif()
    set(MUPFEL_VULKAN_LIBRARY "${MUPFEL_VULKAN_LIB_DIR}/libvulkan.so")
endif()

set(MUPFEL_VULKAN_HEADER_VERSION_FLOOR 357)

set(_mupfel_vulkan_core_header "${MUPFEL_VULKAN_INCLUDE_DIR}/vulkan/vulkan_core.h")
if(NOT EXISTS "${_mupfel_vulkan_core_header}")
    message(FATAL_ERROR
        "Cannot read ${_mupfel_vulkan_core_header}. Is the Vulkan SDK install at ${MUPFEL_VULKAN_SDK} "
        "complete?")
endif()

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

add_library(Mupfel_VulkanHeaders INTERFACE)
add_library(Mupfel::VulkanHeaders ALIAS Mupfel_VulkanHeaders)
target_include_directories(Mupfel_VulkanHeaders SYSTEM INTERFACE "${MUPFEL_VULKAN_INCLUDE_DIR}")

add_library(Mupfel_Vulkan INTERFACE)
add_library(Mupfel::Vulkan ALIAS Mupfel_Vulkan)
target_link_libraries(Mupfel_Vulkan INTERFACE Mupfel::VulkanHeaders "${MUPFEL_VULKAN_LIBRARY}")
