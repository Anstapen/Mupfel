-- App/Build-App.lua
--
-- Builds the "App" executable: game/editor-specific layers (see App/Source) plus the compute/vertex
-- shaders the engine dispatches (see App/Shaders). Links Core (the engine); everything else here is
-- header-only usage (nlohmann json, glm, direct Ping/Vulkan/spdlog types in app-layer code) with no
-- extra linking, since those libraries are already linked in transitively via Core.
--
-- Dependency graph: App -> Core (linked); nlohmann json, glm, Ping, spdlog, Vulkan SDK headers only.

project "App"
    kind "ConsoleApp"
    ApplyDefaultProjectSettings()

    files { "Source/**.h", "Source/**.cpp", "Shaders/**.glsl" }

    includedirs
    {
        "Source",
        "%{wks.location}/Core/Source",
        DepPath("nlohmann"),
        DepPath("glm", "glm"),
        DepPath("ping", "Source"),
        DepPath("spdlog", "include"),
        vulkan_sdk_path .. "/Include",
    }

    links { "Core" }

    filter "system:windows"
        defines { "WINDOWS" }
