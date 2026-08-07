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
--   Ping     -> spdlog/glfw/imgui/stb headers, Vulkan SDK headers
--   box2d    (no internal deps)           (collision detection/resolution, linked by Core)
--   catch2   (no internal deps)           (unit test framework, linked by the Tests project)

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

project "Ping"
    kind "StaticLib"
    ApplyDefaultProjectSettings()

    files { DepPath("ping", "Source/**.h"), DepPath("ping", "Source/**.cpp") }

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

-- Box2D 3.x (unlike the C++ 2.x line) is a pure C library requiring C17 for _Static_assert and
-- anonymous unions, so this is the one project that overrides the C++ language/dialect that
-- ApplyDefaultProjectSettings() applies. Configuration flags mirror upstream's CMake defaults:
-- SIMD on (SSE2 baseline on x64, BOX2D_AVX2 deliberately left off so binaries stay portable),
-- and asserts gated by NDEBUG, which Build.lua already defines for Release/Dist.
project "box2d"
    kind "StaticLib"
    ApplyDefaultProjectSettings()

    language "C"
    cdialect "C17"

    files
    {
        DepPath("box2d", "src/**.c"),
        DepPath("box2d", "src/**.h"),
        DepPath("box2d", "include/**.h"),
    }

    includedirs
    {
        DepPath("box2d", "include"), -- public API, included as <box2d/box2d.h>
        DepPath("box2d", "src"),     -- internal headers, included unqualified by the .c files
    }

    -- The workspace-wide MSVC options in Build.lua are C++-only; MSVC rejects them on a C compiland.
    filter "system:windows"
        removebuildoptions { "/EHsc", "/Zc:__cplusplus" }
        files { DepPath("box2d", "src/box2d.natvis") } -- debugger visualizers for b2Vec2 & friends

    -- Box2D relies on strict IEEE 754 evaluation order for cross-platform determinism, which FMA
    -- contraction breaks (https://box2d.org/posts/2024/08/determinism/). MSVC's default /fp:precise
    -- already disables contraction; GCC/Clang default to -ffp-contract=fast and need this opt-out.
    filter "system:not windows"
        buildoptions { "-ffp-contract=off" }

    filter {}

-- Catch2 v3, built from the two "amalgamated" files upstream ships per release (see Deps.catch2 in
-- Dependencies.lua for why we don't build the full source tree). Only the Tests project links it.
--
-- catch_amalgamated.cpp also defines the test runner's main(), which is why Tests/Source has none of
-- its own. If a test run ever needs engine setup done before the first assertion, define
-- CATCH_AMALGAMATED_CUSTOM_MAIN here and hand-write that main() in the Tests project instead.
--
-- DO_NOT_USE_WMAIN is what keeps that entry point linkable. ApplyDefaultProjectSettings() sets
-- characterset "Unicode", so _UNICODE is defined and Catch2 would emit wmain() rather than main().
-- MSVC picks the CRT entry point (mainCRTStartup vs. wmainCRTStartup) by looking only at the object
-- files on the link line -- never inside a static lib -- so it would settle on mainCRTStartup and
-- then fail with an unresolved "main". Forcing the narrow-char entry point sidesteps that entirely.
project "catch2"
    kind "StaticLib"
    ApplyDefaultProjectSettings()

    files { DepPath("catch2", "catch_amalgamated.hpp"), DepPath("catch2", "catch_amalgamated.cpp") }
    includedirs { DepPath("catch2") }

    defines { "DO_NOT_USE_WMAIN" }
