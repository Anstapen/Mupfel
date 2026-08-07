-- App/Build-App.lua
--
-- Builds the "App" executable: game/editor-specific layers (see App/Source) plus the compute/vertex
-- shaders the engine dispatches (see App/Shaders). Links Core (the engine); spdlog and nlohmann json
-- are header-only usage with no extra linking, since they are already linked in transitively via Core.
--
-- Dependency graph: App -> Core (linked); spdlog and nlohmann json headers only.
--
-- App sees Core/Include, never Core/Source, so engine internals are unreachable rather than merely
-- discouraged. Entry point is #include "Mupfel.h".
--
-- The include path below is the finished state of the vendor-independence work: Ping, the Vulkan SDK,
-- GLFW, ImGui, Box2D and glm are all fully hidden behind Core's public headers and are no longer
-- reachable from application code. Verified by compiling every public header against exactly these
-- three paths and nothing else.
--
-- spdlog and nlohmann json are *deliberately* published rather than leaked -- they are useful to the
-- application author, so Core/Logger.h exposes spdlog loggers and FS/EntityFileManager.h exposes
-- nlohmann::json in its ComponentLoader signature. Both entries are load-bearing: removing either
-- breaks a public header. Do not "clean them up".

project "App"
    kind "ConsoleApp"
    ApplyDefaultProjectSettings()

    files { "Source/**.h", "Source/**.cpp", "Shaders/**.glsl" }

    includedirs
    {
        "Source",
        "%{wks.location}/Core/Include",
        DepPath("spdlog", "include"),
        DepPath("nlohmann"),
    }

    links { "Core" }

    filter "system:windows"
        defines { "WINDOWS" }
