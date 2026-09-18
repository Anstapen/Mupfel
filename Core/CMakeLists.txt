# Core/CMakeLists.txt
#
# Builds the "Core" engine static library: Mupfel's own reusable engine code (ECS, physics/collision
# pipeline, renderer, event system, etc.), see Core/Source. Everything it links against is vendored
# (see Vendor/CMakeLists.txt) or a system dependency (Vulkan SDK, GLFW).
#
# Ping used to sit where nvrhi/nvrhi_vk/vk-bootstrap now are. NVRHI covers only resources, command
# lists, pipelines and resource-state tracking; it explicitly does not create devices or swapchains and
# has no window or ImGui integration, so three things Ping used to hand us now live in Core: the
# vk-bootstrap glue that produces the handles nvrhi::vulkan::createDevice consumes, the
# STB_IMAGE_IMPLEMENTATION translation unit, and the ImGui-on-NVRHI renderer backend. That last one is
# why imgui's backends directory is on the include path and was not before.
#
# ---------------------------------------------------------------------------------------------------
# PUBLIC vs PRIVATE
#
# Headers are split across two roots. "Include" is the published surface, reachable from Mupfel.h;
# "Source" holds engine internals plus every .cpp. In the Premake build that split was enforced purely
# by App's hand-written include path, because Premake has no PUBLIC/PRIVATE model -- CLAUDE.md says so
# explicitly. Here the two mechanisms agree: the directory split still exists, and the link interface
# below states it, so a vendored header cannot leak into App even if someone adds the wrong #include.
#
# Only three things are PUBLIC, and each one is load-bearing:
#   Include/       the published header root.
#   Mupfel::spdlog Core/Include/Core/Logger.h is Mupfel's own logging header and names
#                  spdlog::logger in SafeLoggerPtr.
#   Mupfel::nlohmann  FS/EntityFileManager.h names nlohmann::json in its public ComponentLoader
#                  signature.
# Both vendored ones are *deliberately published*, not leaked -- they are considered useful to the
# application author. Removing either breaks a public header. Don't "clean them up".
#
# Everything else is PRIVATE. Core is a static library, so PRIVATE dependencies still propagate on the
# *link line* (CMake records them as LINK_ONLY) while their include directories and defines do not --
# which is exactly the property that hides nvrhi, vk-bootstrap, the Vulkan SDK, GLFW, ImGui, Box2D and
# glm from application code while still resolving their symbols when App links.
# ---------------------------------------------------------------------------------------------------

file(GLOB_RECURSE core_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h")

add_library(Core STATIC ${core_sources})
mupfel_apply_default_settings(Core)
mupfel_apply_strict_warnings(Core)

target_compile_features(Core PUBLIC cxx_std_23)

# Core needs both roots, and additionally each public subdirectory: existing code includes siblings by
# bare name (#include "SubRenderer.h"), which stops resolving once the sibling moves to the other root.
# Listing the subdirectories keeps that free of source edits. All public header basenames are unique,
# so the flattened search is unambiguous; a future pass that path-qualifies those includes
# ("Renderer/SubRenderer.h") lets the subdirectory entries be dropped again.
#
# Note that only "Include" itself is PUBLIC. The subdirectory entries are PRIVATE, which is what stops
# App from resolving a bare "SubRenderer.h" of its own.
target_include_directories(Core
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/Include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Include/Core"
            "${CMAKE_CURRENT_SOURCE_DIR}/Include/ECS"
            "${CMAKE_CURRENT_SOURCE_DIR}/Include/ECS/Components"
            "${CMAKE_CURRENT_SOURCE_DIR}/Include/FS"
            "${CMAKE_CURRENT_SOURCE_DIR}/Include/Renderer"
            "${CMAKE_CURRENT_SOURCE_DIR}/Source")

# nvrhi_vk before nvrhi mirrors the order the Premake build needed for the GNU linker. CMake computes
# the link line from the dependency graph rather than from this list's order, so it is no longer
# load-bearing -- nvrhi_vk declares nvrhi as a PUBLIC dependency of its own, which is what actually
# guarantees the ordering now.
#
# Mupfel::Vulkan rather than Mupfel::VulkanHeaders: vk-bootstrap needs no Vulkan import library of its
# own (it dlopens the loader), but Core still links it, because glfwCreateWindowSurface and the
# surface/present calls in our own device layer are ordinary prototypes resolved at link time.
target_link_libraries(Core
    PUBLIC  spdlog
            Mupfel::nlohmann
    PRIVATE nvrhi_vk
            nvrhi
            vk-bootstrap
            imgui
            box2d
            Mupfel::Vulkan
            Mupfel::GLFW
            Mupfel::glm
            Mupfel::stb
            # timeBeginPeriod/timeEndPeriod, called from Application.cpp's frame loop. Declared here
            # rather than by a `#pragma comment(lib, "winmm.lib")` in that file, so that every library
            # Core needs is visible in one place. (The pragma is not itself a portability problem --
            # cl, clang-cl and clang all honour it -- it was just the only link dependency hiding in a
            # source file.) Remove the pragma when picking this up; linking it twice is harmless in
            # the meantime.
            $<$<PLATFORM_ID:Windows>:winmm>)

# VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1, VK_USE_PLATFORM_WIN32_KHR and NOMINMAX are deliberately absent
# here: nvrhi_vk publishes all three from its own target, since it is the library whose header forces
# them. The Premake build had to restate them in Core and again in Tests, and nothing but a comment
# kept the three copies in agreement.

set_property(TARGET Core PROPERTY FOLDER "Engine")
