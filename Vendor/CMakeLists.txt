# Vendor/CMakeLists.txt
#
# Builds every vendored third-party dependency, as static-lib targets where there is something to
# compile and interface targets where there isn't. Nothing in this file is Mupfel's own code -- see
# Core/CMakeLists.txt for the engine and App/CMakeLists.txt for the application. The prebuilt GLFW
# binary is the one dependency handled elsewhere, in cmake/GLFW.cmake, because it is a system library
# on Linux and a vendored one only on Windows. stb_image counts as header-only here: Core owns the
# single STB_IMAGE_IMPLEMENTATION translation unit that used to live inside Ping.
#
# Dependency graph of the targets defined below:
#
#   spdlog       (no internal deps)
#   imgui        -> glfw headers only (imgui_impl_glfw backend)
#   nvrhi        (no internal deps)       (NVRHI core: common utilities + the validation layer)
#   nvrhi_vk     -> nvrhi headers, Vulkan SDK headers   (NVRHI's Vulkan backend)
#   vk-bootstrap -> Vulkan SDK headers    (instance/device/queue/swapchain creation, which NVRHI
#                                          deliberately does not do -- see cmake/Dependencies.cmake)
#   box2d        (no internal deps)       (collision detection/resolution, linked by Core)
#   catch2       (no internal deps)       (unit test framework, linked by the Tests target)
#
# All but catch2 are linked by Core, which is part of every possible module selection, so they are
# defined unconditionally. catch2 is created only when the Tests module is (see cmake/Modules.cmake).
#
# Every include directory these targets publish is marked SYSTEM. That is the CMake spelling of
# Premake's externalincludedirs, and the reason Core's /WX and -Werror cannot fire on third-party code.

# --------------------------------------------------------------------------------------------------
# Header-only dependencies
#
# Nothing to compile, but each still gets an interface target so that consumers link a name instead of
# repeating a path -- which is what let Core's and Tests' externalincludedirs lists drift apart in the
# Premake build. All three are included by bare filename ("json.hpp", "stb_image.h") or by the
# directory the archive root already provides (<glm/glm.hpp>), so the include path is the dep dir.
# --------------------------------------------------------------------------------------------------

add_library(Mupfel_nlohmann INTERFACE)
add_library(Mupfel::nlohmann ALIAS Mupfel_nlohmann)
target_include_directories(Mupfel_nlohmann SYSTEM INTERFACE "${MUPFEL_DEP_nlohmann_DIR}")

add_library(Mupfel_glm INTERFACE)
add_library(Mupfel::glm ALIAS Mupfel_glm)
target_include_directories(Mupfel_glm SYSTEM INTERFACE "${MUPFEL_DEP_glm_DIR}")

add_library(Mupfel_stb INTERFACE)
add_library(Mupfel::stb ALIAS Mupfel_stb)
target_include_directories(Mupfel_stb SYSTEM INTERFACE "${MUPFEL_DEP_stb_DIR}")

# --------------------------------------------------------------------------------------------------
# spdlog
# --------------------------------------------------------------------------------------------------

file(GLOB_RECURSE spdlog_sources "${MUPFEL_DEP_spdlog_DIR}/src/*.cpp")
add_library(spdlog STATIC ${spdlog_sources})
mupfel_apply_default_settings(spdlog)

target_compile_features(spdlog PRIVATE cxx_std_23)
target_include_directories(spdlog SYSTEM PUBLIC "${MUPFEL_DEP_spdlog_DIR}/include")

target_compile_definitions(spdlog PUBLIC SPDLOG_COMPILED_LIB)

set_property(TARGET spdlog PROPERTY FOLDER "Vendor")

# --------------------------------------------------------------------------------------------------
# ImGui
# --------------------------------------------------------------------------------------------------

file(GLOB imgui_sources "${MUPFEL_DEP_imgui_DIR}/*.cpp")
add_library(imgui STATIC
    ${imgui_sources}
    "${MUPFEL_DEP_imgui_DIR}/backends/imgui_impl_glfw.cpp")
mupfel_apply_default_settings(imgui)

target_compile_features(imgui PRIVATE cxx_std_23)
target_include_directories(imgui SYSTEM PUBLIC
    "${MUPFEL_DEP_imgui_DIR}"
    "${MUPFEL_DEP_imgui_DIR}/backends")

# Headers only -- the GLFW platform backend includes <GLFW/glfw3.h>. Core links the library itself.
target_link_libraries(imgui PRIVATE Mupfel::GLFW)

set_property(TARGET imgui PROPERTY FOLDER "Vendor")

# --------------------------------------------------------------------------------------------------
# NVRHI
#
# Split into the same two targets upstream's CMakeLists.txt defines, so that adding the D3D12 backend
# later is a new target here rather than a reshuffle of this one. Upstream's build is not invoked: it
# generates nothing (no configure_file, no generated headers), its source lists are flat, and the only
# definitions that matter are the two Windows ones on nvrhi_vk below -- so porting it costs less than
# adopting its option set and install rules. See BUILD.md.
#
# The C++ standard is pinned to 17 (what upstream compiles as) rather than the C++23 our own code uses.
# Same reasoning as box2d overriding the language: we don't fix third-party code, so a dependency bump
# must not be able to break our build over a dialect we picked for our own sources.
# --------------------------------------------------------------------------------------------------

# src/validation is an ordinary opt-in source set upstream (NVRHI_WITH_VALIDATION, default ON) with no
# matching #define -- the validation layer is selected at runtime by wrapping a device in
# nvrhi::validation::createValidationLayer(), so compiling it in costs nothing until it is used.
#
# src/common/dxgi-format.cpp is deliberately absent: upstream compiles it into the D3D backends only,
# and it does not build without the DirectX headers.
file(GLOB nvrhi_validation_sources "${MUPFEL_DEP_nvrhi_DIR}/src/validation/*.cpp")
add_library(nvrhi STATIC
    "${MUPFEL_DEP_nvrhi_DIR}/src/common/format-info.cpp"
    "${MUPFEL_DEP_nvrhi_DIR}/src/common/misc.cpp"
    "${MUPFEL_DEP_nvrhi_DIR}/src/common/state-tracking.cpp"
    "${MUPFEL_DEP_nvrhi_DIR}/src/common/utils.cpp"
    "${MUPFEL_DEP_nvrhi_DIR}/src/common/aftermath.cpp"
    ${nvrhi_validation_sources})
mupfel_apply_default_settings(nvrhi)

target_compile_features(nvrhi PRIVATE cxx_std_17)
target_include_directories(nvrhi SYSTEM PUBLIC "${MUPFEL_DEP_nvrhi_DIR}/include")

# Upstream passes this as a $<BOOL:...> generator expression, which expands to 0 when the feature is
# off. Spelled out because the sources test it with #if, not #ifdef.
target_compile_definitions(nvrhi PUBLIC NVRHI_WITH_AFTERMATH=0)

# Debugger visualizers for nvrhi's RefCountPtr handles and descriptor structs, the same treatment
# box2d.natvis gets. Only the Visual Studio generator consumes these.
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    target_sources(nvrhi PRIVATE "${MUPFEL_DEP_nvrhi_DIR}/tools/nvrhi.natvis")
endif()

set_property(TARGET nvrhi PROPERTY FOLDER "Vendor")

# NVRHI's Vulkan backend. Note that src/vulkan/vulkan-backend.h does its own
#     #define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#     #include <vulkan/vulkan.hpp>
# which has two consequences reaching past this target, both documented in BUILD.md:
#   * every TU in the process that includes <vulkan/vulkan.hpp> must agree on that macro, and exactly
#     one TU may define the dispatcher storage. NVRHI defines it only under NVRHI_SHARED_LIBRARY_BUILD
#     -- see src/vulkan/vulkan-device.cpp:29 -- which a static lib must not set, since nvrhi.h:45 also
#     keys NVRHI_API off it and would dllexport every public symbol. So in this build the storage, and
#     the VULKAN_HPP_DEFAULT_DISPATCHER.init() calls the same #if skips, belong to Core: Renderer.cpp
#     and NVRHIContext::Init respectively. See Reviews/NVRHI-Dispatcher.md.
#   * vulkan.hpp is compiled with exceptions enabled (NVRHI does not define VULKAN_HPP_NO_EXCEPTIONS),
#     which the /EHsc in mupfel_apply_default_settings() already provides on MSVC.
#
# The same header hard-fails on `#if (VK_HEADER_VERSION < 318)`, which is why cmake/VulkanSDK.cmake
# asserts the installed SDK's header version up front rather than letting that surface mid-compile.
file(GLOB nvrhi_vk_sources "${MUPFEL_DEP_nvrhi_DIR}/src/vulkan/*.cpp")
add_library(nvrhi_vk STATIC ${nvrhi_vk_sources})
mupfel_apply_default_settings(nvrhi_vk)

target_compile_features(nvrhi_vk PRIVATE cxx_std_17)
target_link_libraries(nvrhi_vk PUBLIC nvrhi Mupfel::VulkanHeaders)

# PUBLIC, and that is the whole point: it decides what VULKAN_HPP_DEFAULT_DISPATCHER expands to (the
# dynamic loader vs. the static one) and therefore the default dispatch argument -- and the ABI of the
# inline entry points -- on every vk:: call. The Premake build set it by hand on Core, and Tests had to
# reach the same conclusion separately. Publishing it from the target whose header forces it means no
# consumer can disagree, which is the only half of that rule a build system can enforce.
target_compile_definitions(nvrhi_vk PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

if(WIN32)
    # Both mirror upstream, where VK_USE_PLATFORM_WIN32_KHR is PUBLIC for the same reason it is here:
    # it changes what <vulkan/vulkan.h> declares, and every consumer must agree with the TU that
    # compiled NVRHI. It is also what pulls windows.h in, which is what makes NOMINMAX necessary.
    target_compile_definitions(nvrhi_vk PUBLIC VK_USE_PLATFORM_WIN32_KHR NOMINMAX)
endif()

set_property(TARGET nvrhi_vk PROPERTY FOLDER "Vendor")

# --------------------------------------------------------------------------------------------------
# vk-bootstrap
#
# The instance / physical-device / device / queue / swapchain creation that NVRHI leaves to the
# application. One translation unit, C++17, no defines. It resolves Vulkan entry points by dlopening
# the loader at runtime rather than linking it, which is why it takes Mupfel::VulkanHeaders (headers
# only) rather than Mupfel::Vulkan, and why Mupfel::GLFW carries the `dl` it needs on Linux.
# --------------------------------------------------------------------------------------------------

add_library(vk-bootstrap STATIC "${MUPFEL_DEP_vk_bootstrap_DIR}/src/VkBootstrap.cpp")
mupfel_apply_default_settings(vk-bootstrap)

target_compile_features(vk-bootstrap PRIVATE cxx_std_17)
target_include_directories(vk-bootstrap SYSTEM PUBLIC "${MUPFEL_DEP_vk_bootstrap_DIR}/src")
target_link_libraries(vk-bootstrap PUBLIC Mupfel::VulkanHeaders)

if(WIN32)
    target_compile_definitions(vk-bootstrap PUBLIC VK_USE_PLATFORM_WIN32_KHR NOMINMAX)
endif()

set_property(TARGET vk-bootstrap PROPERTY FOLDER "Vendor")

# --------------------------------------------------------------------------------------------------
# Box2D
#
# 3.x (unlike the C++ 2.x line) is a pure C library requiring C17 for _Static_assert and anonymous
# unions, so this is the one target that is not C++ at all. Configuration flags mirror upstream's CMake
# defaults: SIMD on (SSE2 baseline on x64, BOX2D_AVX2 deliberately left off so binaries stay portable),
# and asserts gated by NDEBUG, which the Release flags already define.
# --------------------------------------------------------------------------------------------------

file(GLOB_RECURSE box2d_sources "${MUPFEL_DEP_box2d_DIR}/src/*.c")
add_library(box2d STATIC ${box2d_sources})
mupfel_apply_default_settings(box2d)

target_include_directories(box2d SYSTEM PUBLIC "${MUPFEL_DEP_box2d_DIR}/include")
target_include_directories(box2d PRIVATE "${MUPFEL_DEP_box2d_DIR}/src")

if(WIN32)
    # Debugger visualizers for b2Vec2 & friends.
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        target_sources(box2d PRIVATE "${MUPFEL_DEP_box2d_DIR}/src/box2d.natvis")
    endif()

    # MSVC knows only stdc11/stdc17/stdclatest.
    set_target_properties(box2d PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
else()
    # gnu17 rather than c17, matching upstream's C_EXTENSIONS YES. Strict -std=c17 defines
    # __STRICT_ANSI__, which makes glibc hide everything POSIX -- and src/timer.c's Linux branch calls
    # clock_gettime(CLOCK_MONOTONIC) and sched_yield().
    set_target_properties(box2d PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED ON C_EXTENSIONS ON)
endif()

# Box2D relies on strict IEEE 754 evaluation order for cross-platform determinism, which FMA
# contraction breaks (https://box2d.org/posts/2024/08/determinism/).
#
# Keyed on the compiler, not the platform. It used to sit in the `else()` of the `if(WIN32)` above,
# which meant a Clang build on Windows matched neither branch and silently kept contraction enabled --
# the one configuration where the opt-out is both needed and absent.
#
# The three spellings are not interchangeable, and two of the three failure modes are silent:
#   MSVC       /fp:precise is the default and already disables contraction. Passing the GCC-style flag
#              earns `D9002 unknown option ... ignored`.
#   clang-cl   ignores a bare -ffp-contract=off ("unknown argument ignored in clang-cl"), so it needs
#              the /clang: pass-through. Verified: the bare form is dropped, this form is accepted.
#   clang/gcc  take the flag directly.
#
# In practice contraction cannot occur on the SSE2 baseline this builds for -- there is no FMA
# instruction to contract into -- so this is insurance for anyone who turns on BOX2D_AVX2 later. That
# is exactly when a silently-dropped flag would start costing determinism.
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    # Nothing to do; /fp:precise is already the default.
elseif(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(box2d PRIVATE /clang:-ffp-contract=off)
else()
    target_compile_options(box2d PRIVATE -ffp-contract=off)
endif()

# Box2D is the one target that stays optimized in Debug: an unoptimized solver/broadphase makes the
# game too slow to debug, and this is third-party code nobody steps through anyway. Everything else
# gets the configuration's own optimization level. These options land after the per-configuration
# flags on the command line, so the later setting is the one that takes effect.
if(MSVC)
    target_compile_options(box2d PRIVATE $<$<CONFIG:Debug>:/O2>)

    # /RTC1 together with /O2 is a hard "error D8016: incompatible options", and MSVC's runtime checks
    # are on by default in Debug. Clearing the property is what removes the flag; it cannot be undone
    # by adding a compile option, since there is no negative form of /RTC1.
    set_property(TARGET box2d PROPERTY MSVC_RUNTIME_CHECKS "")
else()
    target_compile_options(box2d PRIVATE $<$<CONFIG:Debug>:-O3>)
endif()

set_property(TARGET box2d PROPERTY FOLDER "Vendor")

# --------------------------------------------------------------------------------------------------
# Catch2
#
# v3, built from the two "amalgamated" files upstream ships per release (see the catch2 entry in
# cmake/Dependencies.cmake for why we don't build the full source tree). Only the Tests target links it.
#
# catch_amalgamated.cpp also defines the test runner's main(), which is why Tests/Source has none of
# its own. If a test run ever needs engine setup done before the first assertion, define
# CATCH_AMALGAMATED_CUSTOM_MAIN here and hand-write that main() in the Tests target instead.
#
# DO_NOT_USE_WMAIN is what keeps that entry point linkable. mupfel_apply_default_settings() defines
# _UNICODE on MSVC, so Catch2 would otherwise emit wmain() rather than main(). MSVC picks the CRT entry
# point (mainCRTStartup vs. wmainCRTStartup) by looking only at the object files on the link line --
# never inside a static lib -- so it would settle on mainCRTStartup and then fail with an unresolved
# "main". Forcing the narrow-char entry point sidesteps that entirely.
# --------------------------------------------------------------------------------------------------

if(MUPFEL_MODULE_TESTS)
    add_library(catch2 STATIC "${MUPFEL_DEP_catch2_DIR}/catch_amalgamated.cpp")
    mupfel_apply_default_settings(catch2)

    target_compile_features(catch2 PRIVATE cxx_std_23)
    target_include_directories(catch2 SYSTEM PUBLIC "${MUPFEL_DEP_catch2_DIR}")
    target_compile_definitions(catch2 PRIVATE DO_NOT_USE_WMAIN)

    set_property(TARGET catch2 PROPERTY FOLDER "Vendor")
endif()
