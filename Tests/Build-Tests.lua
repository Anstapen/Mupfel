-- Tests/Build-Tests.lua
--
-- Builds the "Tests" executable: unit tests for the engine, built on Catch2 v3 (see Deps.catch2 in
-- Dependencies.lua and the catch2 project in Vendor/Build-Vendor.lua). Links Core and catch2.
--
-- The include/lib dirs mirror Core/Build-Core.lua rather than App/Build-App.lua for the same reason
-- Benchmarks does: anything reaching into Core/Application.h pulls in the full Ping/Vulkan/GLFW/spdlog/
-- imgui header chain behind it, so those paths have to be visible here even though nothing but Core
-- and catch2 is linked.
--
-- Two source roots, both globbed and both on the include path, so adding a file to either needs no
-- edit here -- only a re-run of the setup script so Premake regenerates the project:
--   Source/  the tests themselves (Test_*.cpp)
--   Common/  test-only helpers shared between them (Random.h), deliberately kept out of Core since
--            nothing the engine ships depends on them

project "Tests"
    kind "ConsoleApp"
    ApplyDefaultProjectSettings()

    files { "Source/**.h", "Source/**.cpp", "Common/**.h", "Common/**.cpp" }

    includedirs
    {
        "Source",
        "Common",
        -- Unlike App, the tests are white-box: they exercise engine internals (Core/ResourceManager.h)
        -- and so get both of Core's header roots rather than just the published one.
        "%{wks.location}/Core/Include",
        "%{wks.location}/Core/Source",
        DepPath("catch2"),
        DepPath("nlohmann"),
        DepPath("ping", "Source"),
        vulkan_sdk_path .. "/Include",
        DepPath("glfw", "include"),
        DepPath("spdlog", "include"),
        DepPath("imgui"),
    }

    libdirs
    {
        DepPath("glfw", "lib-vc2022"),
        vulkan_sdk_path .. "/Lib",
    }

    links { "Core", "catch2" }

    filter "system:windows"
        defines { "WINDOWS" }
    filter {}
