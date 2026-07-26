-- Benchmarks/Build-Benchmarks.lua
--
-- Builds the "Benchmarks" executable: microbenchmarks for the engine's hot paths (ECS registry,
-- View iteration, component access, ParallelForEach), built on the header-only nanobench framework
-- (see Dependencies.lua -> Deps.nanobench). Links Core, exactly like App does.
--
-- The include/lib dirs mirror Core/Build-Core.lua rather than App/Build-App.lua on purpose: the
-- ParallelForEach benchmark instantiates Registry::ParallelForEach, whose template body references
-- Application::GetCurrentThreadPool(), so the benchmark TU pulls in Core/Application.h and, through
-- it, the full Ping/Vulkan/GLFW/spdlog/imgui header chain. Constructing the Application singleton at
-- runtime only default-constructs it (no window / no Vulkan init happens until Application::Init()),
-- so the benchmarks run headless.
--
-- Reliability note: only the Release and Dist configurations produce meaningful numbers. Debug builds
-- disable optimization (and enable the ECS asserts), so treat Debug benchmark output as a smoke test
-- of the harness, not as performance data.

project "Benchmarks"
    kind "ConsoleApp"
    ApplyDefaultProjectSettings()

    files { "Source/**.h", "Source/**.cpp" }

    includedirs
    {
        "Source",
        "%{wks.location}/Core/Source",
        DepPath("nanobench"),
        DepPath("nlohmann"),
        DepPath("glm", "glm"),
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

    links { "Core" }

    filter "system:windows"
        defines { "WINDOWS" }
    filter {}
