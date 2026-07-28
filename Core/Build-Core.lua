-- Core/Build-Core.lua
--
-- Builds the "Core" engine static library: Mupfel's own reusable engine code (ECS, physics/collision
-- pipeline, renderer, event system, etc.), see Core/Source. Everything it links against is vendored
-- (see Vendor/Build-Vendor.lua) or a system dependency (Vulkan SDK, GLFW prebuilt binary).
--
-- Dependency graph: Core -> Ping, Logger, spdlog, imgui, box2d (linked); glfw3, vulkan (prebuilt/system,
-- linked via libdirs); nlohmann json headers only (entity (de)serialization, no link needed).

project "Core"
    kind "StaticLib"
    ApplyDefaultProjectSettings()

    files { "Source/**.h", "Source/**.cpp" }

    includedirs
    {
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
        "Logger",
        "spdlog",
        "imgui",
        "glfw3",
        "vulkan",
        "box2d",
    }
