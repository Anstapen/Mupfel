-- premake5.lua
--
-- Workspace layout (see BUILD.md for the full dependency-graph writeup):
--   Vendor group  -> Vendor/Build-Vendor.lua  (third-party libraries built from vendored source)
--   Engine group  -> Core/Build-Core.lua      (Mupfel's own engine static library, "Core")
--   (ungrouped)   -> App/Build-App.lua        (the executable, startproject)

-- Ping (Vendor/Build-Vendor.lua) needs the Vulkan SDK to compile
vulkan_sdk_path = os.getenv("VULKAN_SDK")
if not vulkan_sdk_path then
   error("VULKAN_SDK not set. Please set this variable to the path of the installed Vulkan SDK. Exiting...")
end

include "Dependencies.lua"

workspace "Mupfel"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "App"

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

    filter "action:vs*"
        defines { "_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS" }
        characterset "Unicode"

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    -- NDEBUG disables assert() in the optimized configurations. The ECS hot paths assert per entity
    -- (Registry::GetSignature) and per component access (CPUComponentArray::Get -> Has), so leaving
    -- them enabled costs measurable time in View iteration. Premake does not define NDEBUG on its own.
    filter "configurations:Release"
        defines { "RELEASE", "NDEBUG" }
        runtime "Release"
        optimize "On"
        symbols "On"

    filter "configurations:Dist"
        defines { "DIST", "NDEBUG" }
        runtime "Release"
        optimize "On"
        symbols "Off"

    filter {}
end

group "Vendor"
   include "Vendor/Build-Vendor.lua"
group ""

group "Engine"
   include "Core/Build-Core.lua"
group ""

include "App/Build-App.lua"

group "Benchmarks"
   include "Benchmarks/Build-Benchmarks.lua"
group ""
