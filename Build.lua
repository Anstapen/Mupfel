-- premake5.lua

-- Ping (Core/Ping.lua) needs the Vulkan SDK to compile
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

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "Core"
	include "Core/Build-Core.lua"
group ""

include "App/Build-App.lua"