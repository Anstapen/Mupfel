-- Core/Build-Core.lua
--
-- Builds the "Core" engine static library: Mupfel's own reusable engine code (ECS, physics/collision
-- pipeline, renderer, event system, etc.), see Core/Source. Everything it links against is vendored
-- (see Vendor/Build-Vendor.lua) or a system dependency (Vulkan SDK, GLFW prebuilt binary).
--
-- Dependency graph: Core -> Ping, spdlog, imgui, box2d (linked); glfw3, vulkan (prebuilt/system,
-- linked via libdirs); nlohmann json headers only (entity (de)serialization, no link needed).
-- Core no longer links "Logger" directly -- Core/Include/Core/Logger.h is Mupfel's own spdlog wrapper.
-- The Logger vendor project still builds because Ping links it (see Vendor/Build-Vendor.lua).
--
-- Note: glm has no includedirs entry, yet Core uses it in five files (Camera.cpp, ECSRenderer.cpp,
-- IMRenderer.cpp, DebugLayer.cpp, Renderer/Quad.h). It resolves only because the LunarG Vulkan SDK
-- ships glm under its own Include directory, which is on the path above. Vendor/Sources/glm-master is
-- downloaded by Dependencies.lua and unused. Add DepPath("glm", "glm") here if a Vulkan SDK without
-- bundled glm ever breaks the build.
--
-- Headers are split across two roots. "Include" is the published surface, reachable from Mupfel.h and
-- the only root App puts on its include path (see App/Build-App.lua). "Source" holds engine internals
-- plus every .cpp, and App cannot see it -- that directory split IS the access control, since Premake
-- has no PUBLIC/PRIVATE include model.
--
-- Core itself needs both roots, and additionally each public subdirectory: existing code includes
-- siblings by bare name (#include "SubRenderer.h"), which stops resolving once the sibling moves to
-- the other root. Listing the subdirectories keeps the move free of source edits. All 41 public header
-- basenames are unique, so the flattened search is unambiguous; a future pass that path-qualifies
-- those includes ("Renderer/SubRenderer.h") lets the subdirectory entries be dropped again.

project "Core"
    kind "StaticLib"
    ApplyDefaultProjectSettings()

    files { "Include/**.h", "Source/**.h", "Source/**.cpp" }

    includedirs
    {
        "Include",
        "Include/Core",
        "Include/ECS",
        "Include/ECS/Components",
        "Include/FS",
        "Include/Renderer",
        "Source",
        DepPath("nlohmann"),
        DepPath("ping", "Source"),
        vulkan_sdk_path .. "/Include",
        DepPath("glfw", "include"),
        DepPath("spdlog", "include"),
        DepPath("imgui"),
        DepPath("box2d", "include"),
    }

    libdirs
    {
        DepPath("glfw", "lib-vc2022"),
        vulkan_sdk_path .. "/Lib",
    }

    links
    {
        "Ping",
        "spdlog",
        "imgui",
        "glfw3",
        "vulkan",
        "box2d",
    }
