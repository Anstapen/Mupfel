-- premake5.lua
--
-- Workspace layout (see BUILD.md for the full dependency-graph writeup):
--   Vendor group     -> Vendor/Build-Vendor.lua          (third-party libraries built from vendored source)
--   Engine group     -> Core/Build-Core.lua              (Mupfel's own engine static library, "Core")
--   (ungrouped)      -> App/Build-App.lua                (the executable, startproject)
--   Tests group      -> Tests/Build-Tests.lua            (Catch2 unit tests, links Core)
--
-- Everything below Vendor is conditional: --modules=core,app generates a solution with just those
-- two, and the default remains all four. The option, the module table and the dependency closure
-- that keeps a selection buildable all live in Modules.lua.

-- Ping (Vendor/Build-Vendor.lua) needs the Vulkan SDK to compile
vulkan_sdk_path = os.getenv("VULKAN_SDK")
if not vulkan_sdk_path then
   error("VULKAN_SDK not set. Please set this variable to the path of the installed Vulkan SDK. Exiting...")
end

-- Modules.lua first: it parses --modules, and Dependencies.lua skips fetching the frameworks whose
-- module isn't in this solution.
include "Modules.lua"
include "Dependencies.lua"

workspace "Mupfel"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }

   -- App when it's in the solution, otherwise the first runnable module. Left unset for a Core-only
   -- solution: a startproject naming a project that was never generated confuses the IDE.
   local start_project = StartProjectName()
   if start_project then
      startproject (start_project)
   end

   -- Workspace-wide build options for MSVC
   -- spdlog's bundled fmt headers require an UTF-8 execution charset (their own headers are valid
   -- UTF-8). We deliberately don't force /source-charset:utf-8 (which /utf-8 would also imply) since
   -- some of our own headers aren't valid UTF-8 and MSVC would fail/warn (C4828) trying to parse them
   -- as such.
   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus", "/execution-charset:utf-8" }
   filter {}

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

-- Shared by every project (vendor, engine, app) so toolchain/output settings live in exactly one
-- place instead of being copy-pasted per project file.
function ApplyDefaultProjectSettings()
    language "C++"
    cppdialect "C++23"
    staticruntime "off"

    targetdir ("%{wks.location}/Binaries/" .. OutputDir .. "/%{prj.name}")
    objdir ("%{wks.location}/Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

    -- glm bakes the clip-space depth convention into ortho()/perspective() at compile time via
    -- GLM_CONFIG_CLIP_CONTROL. Those are inline functions, so every TU must agree: if one TU sees
    -- this define and another doesn't, the two differing bodies are an ODR violation and the linker
    -- silently keeps whichever it saw first. Vulkan clips to 0 <= z <= w, so it must be [0,1] --
    -- getting OpenGL's [-1,1] makes the whole scene fail the depth test and render black.
    -- Set here rather than per-file so a stray `#include <glm/glm.hpp>` can't opt a TU out.
    defines { "GLM_FORCE_DEPTH_ZERO_TO_ONE" }

    filter "action:vs*"
        defines { "_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS" }
        characterset "Unicode"

    filter "system:windows"
        systemversion "latest"

    -- optimize "Off" is stated rather than left to Premake's default: Debug is for stepping through
    -- code that still matches the source, so nothing here may be reordered or inlined away.
    --   optimize "Off"   -> -O0 (clang/gcc) | /Od (MSVC, i.e. <Optimization>Disabled)
    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"
        optimize "Off"

    -- NDEBUG disables assert() in the optimized configurations. The ECS hot paths assert per entity
    -- (Registry::GetSignature) and per component access (CPUComponentArray::Get -> Has), so leaving
    -- them enabled costs measurable time in View iteration. Premake does not define NDEBUG on its own.
    --
    -- optimize "Speed", not "On": the two verbs differ per toolset, and Speed is the one that means
    -- "optimize for speed" on both.
    --   optimize "Speed" -> -O3 (clang/gcc) | /O2 (MSVC, i.e. <Optimization>MaxSpeed)
    --   optimize "On"    -> -O2 (clang/gcc) | /Ox (MSVC, a *subset* of /O2)
    filter "configurations:Release"
        defines { "RELEASE", "NDEBUG" }
        runtime "Release"
        optimize "Speed"
        symbols "On"

    -- Same code generation as Release; Dist only drops the debug symbols.
    filter "configurations:Dist"
        defines { "DIST", "NDEBUG" }
        runtime "Release"
        optimize "Speed"
        symbols "Off"

    filter {}
end

-- Strict warnings, applied only to Mupfel's own compiled code (Core, Tests). Deliberately
-- NOT part of ApplyDefaultProjectSettings(): every project calls that one, the vendored ones included,
-- and third-party source keeps whatever warning level its authors settled on -- we don't fix their
-- code, and a dependency bump must not be able to break our build.
--
-- Spelled with Premake's portable verbs rather than raw compiler flags, because these same scripts
-- generate both an MSVC solution and clang makefiles:
--   warnings "High"         -> -Wall   (clang/gcc) | /W4 (MSVC, i.e. <WarningLevel>Level4)
--   fatalwarnings { "All" } -> -Werror (clang/gcc) | /WX (MSVC, i.e. <TreatWarningAsError>)
--   externalwarnings "Off"  -> see below           | /external:W0
--
-- The other half of "ignore vendored warnings" is include *paths*: a third-party header included from
-- one of our .cpp files warns as if we had written it, so -Werror would fail our build over spdlog's
-- or ImGui's code. The three projects therefore list every vendored path under `externalincludedirs`
-- (emits -isystem, and <ExternalIncludePath> + the external warning level above) instead of
-- `includedirs`, which silences warnings originating inside those headers. Search order is unchanged:
-- both -isystem and MSVC's external paths are searched after the normal include dirs, which is exactly
-- where the vendored entries already sat.
function ApplyStrictWarnings()
    warnings "High"
    externalwarnings "Off"
    fatalwarnings { "All" }
end

-- Vendor is unconditional: every possible --modules selection contains Core (see Modules.lua), and
-- Core links Ping, spdlog, imgui and box2d. catch2 is the one vendored project that isn't always
-- needed, and it gates itself inside Vendor/Build-Vendor.lua.
group "Vendor"
   include "Vendor/Build-Vendor.lua"
group ""

-- Our own modules, in ModuleOrder order, restricted to what --modules asked for.
for _, module in ipairs(SelectedModules()) do
   group (Modules[module].group)
      include (Modules[module].build_script)
   group ""
end
