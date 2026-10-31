cmake_minimum_required(VERSION 4.0)

project(Mupfel LANGUAGES C CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "In-source builds are not supported. Configure into a subdirectory, e.g. "
        "`cmake --preset windows`, which builds into Build/windows.")
endif()

set(MUPFEL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${MUPFEL_ROOT}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

get_property(_mupfel_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_mupfel_multi_config)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
else()
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
    endif()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

if(MSVC)
    foreach(kind IN ITEMS EXE SHARED MODULE)
        string(APPEND CMAKE_${kind}_LINKER_FLAGS_DEBUG " /DEBUG")
        string(APPEND CMAKE_${kind}_LINKER_FLAGS_RELEASE " /DEBUG /OPT:REF /OPT:ICF")
    endforeach()
else()
    string(APPEND CMAKE_C_FLAGS_RELEASE " -g")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -g")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")

include(Modules)
include(Dependencies)
include(MupfelSettings)
include(VulkanSDK)

mupfel_fetch_all()
include(GLFW)

enable_testing()

add_subdirectory(Vendor)

foreach(module IN LISTS MUPFEL_SELECTED_MODULES)
    add_subdirectory("${MUPFEL_${module}_DIR}")
endforeach()

if(MUPFEL_START_PROJECT)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "${MUPFEL_START_PROJECT}")
endif()
