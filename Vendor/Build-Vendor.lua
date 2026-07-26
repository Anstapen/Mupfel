-- Vendor/Build-Vendor.lua
--
-- Builds every vendored third-party dependency that needs compiling, as static-lib projects. Nothing
-- in this file is Mupfel's own code — see Core/Build-Core.lua for the engine and App/Build-App.lua for
-- the application. Header-only deps (nlohmann json, glm, stb_image) and the prebuilt GLFW binary need
-- no project here; they're just include/lib paths consumed directly via Deps/DepPath (Dependencies.lua).
--
-- Dependency graph of the projects defined below:
--
--   spdlog   (no internal deps)
--   imgui    -> glfw headers only (imgui_impl_glfw backend)
--   Logger   -> spdlog headers            (Logger is split out of Ping's own source tree so it can be
--                                           linked independently by both Ping and Core)
--   Ping     -> Logger (linked), spdlog/glfw/imgui/stb headers, Vulkan SDK headers

project "spdlog"
    kind "StaticLib"
    ApplyDefaultProjectSettings()

    files { DepPath("spdlog", "include/**.h"), DepPath("spdlog", "src/**.cpp") }
    includedirs { DepPath("spdlog", "include") }

    defines { "SPDLOG_COMPILED_LIB" }

project "imgui"
    kind "StaticLib"
    ApplyDefaultProjectSettings()

    files
    {
        DepPath("imgui", "*.h"),
        DepPath("imgui", "*.cpp"),
        DepPath("imgui", "backends/imgui_impl_glfw.h"),
        DepPath("imgui", "backends/imgui_impl_glfw.cpp"),
    }

    includedirs
    {
        DepPath("imgui"),
        DepPath("imgui", "backends"),
        DepPath("glfw", "include"),
    }

project "Logger"
    kind "StaticLib"
    ApplyDefaultProjectSettings()

    files { DepPath("ping", "Source/Logger/**.h"), DepPath("ping", "Source/Logger/**.cpp") }

    includedirs
    {
        DepPath("ping", "Source"),
        DepPath("spdlog", "include"),
    }

project "Ping"
    kind "StaticLib"
    ApplyDefaultProjectSettings()

    files { DepPath("ping", "Source/**.h"), DepPath("ping", "Source/**.cpp") }
    -- Logger is its own project (above) so it can be linked independently of the rest of Ping.
    removefiles { DepPath("ping", "Source/Logger/**.h"), DepPath("ping", "Source/Logger/**.cpp") }

    includedirs
    {
        DepPath("ping", "Source"),
        vulkan_sdk_path .. "/Include", -- system dependency (VULKAN_SDK env var), not vendored/fetched
        DepPath("glfw", "include"),
        DepPath("spdlog", "include"),
        DepPath("stb"),
        DepPath("imgui"),
        DepPath("imgui", "backends"),
    }

    links { "Logger" }
