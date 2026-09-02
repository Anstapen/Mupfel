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
-- Note: glm is vendored (Deps.glm in Dependencies.lua) and listed under externalincludedirs below.
-- It used to have no entry at all and resolved only because the LunarG *Windows* SDK happens to ship
-- a copy under its own Include directory -- which quietly made a full SDK install a requirement of
-- the build on every platform, glm having nothing to do with Vulkan. The path is DepPath("glm") and
-- not DepPath("glm", "glm"): the archive root already contains the glm/ directory that the five
-- users of it (Camera.cpp, ECSRenderer.cpp, IMRenderer.cpp, DebugLayer.cpp, Renderer/Quad.h) name
-- as <glm/glm.hpp>.
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
    ApplyStrictWarnings()

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
    }

    -- Vendored/system headers, kept out of includedirs so ApplyStrictWarnings()'s -Werror / /WX can't
    -- fail the build over third-party code: these emit -isystem (clang/gcc) and <ExternalIncludePath>
    -- (MSVC), both compiled at the "Off" external warning level. See Build.lua.
    externalincludedirs
    {
        DepPath("nlohmann"),
        DepPath("glm"),
        DepPath("ping", "Source"),
        VulkanIncludeDir,
        -- Windows-only prebuilt GLFW (Deps.glfw is windows_only). On Linux this path does not exist
        -- and the system headers installed by libglfw3-dev are found on the default search path.
        DepPath("glfw", "include"),
        DepPath("spdlog", "include"),
        DepPath("imgui"),
        DepPath("box2d", "include"),
    }

    libdirs
    {
        VulkanLibDir,
    }

    links
    {
        "Ping",
        "spdlog",
        "imgui",
        VulkanLibName,
        "box2d",
    }

    -- GLFW is the one dependency that is not vendored the same way on both platforms, so it cannot be
    -- linked unconditionally: Windows uses the prebuilt binary fetched as Deps.glfw, Linux links the
    -- system libglfw.so from libglfw3-dev -- which is -lglfw, not -lglfw3.
    filter "system:windows"
        libdirs { DepPath("glfw", "lib-vc2022") }
        links   { "glfw3" }

    filter "system:not windows"
        links   { "glfw" }

    filter {}
