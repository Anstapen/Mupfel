-- Vendor/Build-Vendor.lua
--
-- Builds every vendored third-party dependency that needs compiling, as static-lib projects. Nothing
-- in this file is Mupfel's own code — see Core/Build-Core.lua for the engine and App/Build-App.lua for
-- the application. Header-only deps (nlohmann json, glm, stb_image) and the prebuilt GLFW binary need
-- no project here; they're just include/lib paths consumed directly via Deps/DepPath (Dependencies.lua).
-- stb_image counts as header-only here too: Core now owns the single STB_IMAGE_IMPLEMENTATION
-- translation unit that used to live inside Ping.
--
-- Ping, Mupfel's own Vulkan wrapper, used to be defined in this file. It was replaced by NVRHI
-- (resources, command lists, pipelines, automatic resource-state tracking) plus vk-bootstrap (the
-- instance/device/swapchain creation NVRHI deliberately does not do). See BUILD.md for that split.
--
-- Dependency graph of the projects defined below:
--
--   spdlog       (no internal deps)
--   imgui        -> glfw headers only (imgui_impl_glfw backend)
--   nvrhi        (no internal deps)       (NVRHI core: common utilities + the validation layer)
--   nvrhi_vk     -> nvrhi headers, Vulkan SDK headers   (NVRHI's Vulkan backend)
--   vk-bootstrap -> Vulkan SDK headers    (instance/device/queue/swapchain creation, which NVRHI
--                                          deliberately does not do -- see Dependencies.lua)
--   box2d        (no internal deps)       (collision detection/resolution, linked by Core)
--   catch2       (no internal deps)       (unit test framework, linked by the Tests project)
--
-- All but catch2 are linked by Core, which is part of every possible --modules selection, so they are
-- defined unconditionally. catch2 is generated only when the Tests module is (see Modules.lua).

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

-- NVRHI, split into the same two targets upstream's CMakeLists.txt defines, so that adding the D3D12
-- backend later is a new project here rather than a reshuffle of this one. Upstream's build is not
-- invoked: it generates nothing (no configure_file, no generated headers), its source lists are flat,
-- and the only definitions that matter are the two Windows ones on nvrhi_vk below -- so porting it
-- costs less than making CMake a build requirement. See BUILD.md.
--
-- cppdialect is pinned to C++17 (what upstream compiles as) rather than inheriting the workspace's
-- C++23 from ApplyDefaultProjectSettings(). Same reasoning as box2d overriding `language`: we don't
-- fix third-party code, so a dependency bump must not be able to break our build over a dialect we
-- picked for our own sources.
project "nvrhi"
    kind "StaticLib"
    ApplyDefaultProjectSettings()
    cppdialect "C++17"

    -- src/validation is an ordinary opt-in source set upstream (NVRHI_WITH_VALIDATION, default ON)
    -- with no matching #define -- the validation layer is selected at runtime by wrapping a device in
    -- nvrhi::validation::createValidationLayer(), so compiling it in costs nothing until it is used.
    --
    -- src/common/dxgi-format.cpp is deliberately absent: upstream compiles it into the D3D backends
    -- only, and it does not build without the DirectX headers.
    files
    {
        DepPath("nvrhi", "include/nvrhi/**.h"),
        DepPath("nvrhi", "src/common/format-info.cpp"),
        DepPath("nvrhi", "src/common/misc.cpp"),
        DepPath("nvrhi", "src/common/state-tracking.cpp"),
        DepPath("nvrhi", "src/common/utils.cpp"),
        DepPath("nvrhi", "src/common/aftermath.cpp"),
        DepPath("nvrhi", "src/common/*.h"),
        DepPath("nvrhi", "src/validation/**.cpp"),
        DepPath("nvrhi", "src/validation/**.h"),
    }

    includedirs { DepPath("nvrhi", "include") }

    -- Upstream passes this as a $<BOOL:...> generator expression, which expands to 0 when the feature
    -- is off. Spelled out because the sources test it with #if, not #ifdef.
    defines { "NVRHI_WITH_AFTERMATH=0" }

    -- Debugger visualizers for nvrhi's RefCountPtr handles and descriptor structs, the same treatment
    -- box2d.natvis gets. MSVC-only, so it hangs off the vs* action rather than system:windows.
    filter "action:vs*"
        files { DepPath("nvrhi", "tools/nvrhi.natvis") }

    filter {}

-- NVRHI's Vulkan backend. Note that src/vulkan/vulkan-backend.h does its own
--     #define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
--     #include <vulkan/vulkan.hpp>
-- which has two consequences reaching past this project, both documented in BUILD.md:
--   * every TU in the process that includes <vulkan/vulkan.hpp> must agree on that macro (Core sets
--     VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 project-wide for exactly that reason), and exactly one TU
--     may define the dispatcher storage. NVRHI defines it only under NVRHI_SHARED_LIBRARY_BUILD --
--     see src/vulkan/vulkan-device.cpp:29 -- which a StaticLib must not set, since nvrhi.h:45 also
--     keys NVRHI_API off it and would dllexport every public symbol. So in this build the storage,
--     and the VULKAN_HPP_DEFAULT_DISPATCHER.init() calls the same #if skips, belong to Core:
--     Renderer.cpp and NVRHIContext::Init respectively. See Reviews/NVRHI-Dispatcher.md.
--   * vulkan.hpp is compiled with exceptions enabled (NVRHI does not define VULKAN_HPP_NO_EXCEPTIONS),
--     which the workspace-wide /EHsc in Build.lua already provides on MSVC.
--
-- The same header hard-fails on `#if (VK_HEADER_VERSION < 318)`, which is why Build.lua asserts the
-- installed SDK's header version up front rather than letting that surface as an #error mid-compile.
project "nvrhi_vk"
    kind "StaticLib"
    ApplyDefaultProjectSettings()
    cppdialect "C++17"

    files { DepPath("nvrhi", "src/vulkan/**.cpp"), DepPath("nvrhi", "src/vulkan/**.h") }

    includedirs
    {
        DepPath("nvrhi", "include"),
        VulkanIncludeDir, -- system dependency (VULKAN_SDK env var), not vendored/fetched
    }

    defines { "NVRHI_WITH_AFTERMATH=0" }

    -- Both mirror upstream. VK_USE_PLATFORM_WIN32_KHR is PUBLIC there, so Core repeats it (see
    -- Core/Build-Core.lua): it changes what <vulkan/vulkan.h> declares, and the two must agree. It is
    -- also what pulls windows.h in, which is what makes NOMINMAX necessary.
    filter "system:windows"
        defines { "VK_USE_PLATFORM_WIN32_KHR", "NOMINMAX" }

    filter {}

-- vk-bootstrap: the instance / physical-device / device / queue / swapchain creation that NVRHI leaves
-- to the application. One translation unit, C++17, no defines. It resolves Vulkan entry points by
-- dlopening the loader at runtime rather than linking it, so it adds nothing to links{} here -- Core
-- links `dl` on Linux on its behalf (see Core/Build-Core.lua).
project "vk-bootstrap"
    kind "StaticLib"
    ApplyDefaultProjectSettings()
    cppdialect "C++17"

    files
    {
        DepPath("vk_bootstrap", "src/VkBootstrap.cpp"),
        DepPath("vk_bootstrap", "src/*.h"),
        DepPath("vk_bootstrap", "src/*.inl"),
    }

    includedirs
    {
        DepPath("vk_bootstrap", "src"),
        VulkanIncludeDir,
    }

    filter "system:windows"
        defines { "VK_USE_PLATFORM_WIN32_KHR", "NOMINMAX" }

    filter {}

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
    --
    -- gnu17 rather than C17 there, matching upstream's C_EXTENSIONS YES. Strict -std=c17 defines
    -- __STRICT_ANSI__, which makes glibc hide everything POSIX -- and src/timer.c's Linux branch calls
    -- clock_gettime(CLOCK_MONOTONIC) and sched_yield(). Linux-only because Premake's MSVC generator
    -- knows only stdc11/stdc17/stdclatest: gnu17 there would emit no LanguageStandard_C at all.
    filter "system:not windows"
        buildoptions { "-ffp-contract=off" }
        cdialect "gnu17"

    -- Box2D is the one project that stays optimized in Debug: an unoptimized solver/broadphase makes
    -- the game too slow to debug, and this is third-party code nobody steps through anyway. Build.lua
    -- gives every other project optimize "Off" there.
    --
    -- On MSVC that combination needs the runtime-check opt-out. MSBuild's CL task turns on /RTC1
    -- whenever the runtime is Debug, and /RTC1 together with /O2 is a hard "error D8016: incompatible
    -- options" -- so this configuration failed to compile at all until runtimechecks "Off" was added.
    -- Premake does not infer the opt-out from the optimize level.
    filter "configurations:Debug"
        optimize "Speed"
        runtimechecks "Off"

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
if ModuleSelected("tests") then
    project "catch2"
        kind "StaticLib"
        ApplyDefaultProjectSettings()

        files { DepPath("catch2", "catch_amalgamated.hpp"), DepPath("catch2", "catch_amalgamated.cpp") }
        includedirs { DepPath("catch2") }

        defines { "DO_NOT_USE_WMAIN" }
end
