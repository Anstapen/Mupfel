
include "Ping.lua"

project "Core"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp" }

   includedirs
   {
      "Source"
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")
   links {
          "Ping",
          "Logger",
          "spdlog",
          "imgui",
          "glfw3",
          "vulkan"
         }

   libdirs {"../" .. glfw_dir .. "/lib-vc2022"}
   libdirs {vulkan_sdk_path .. "/Lib"}

   includedirs {"../Vendor/Sources/nlohmann"}
   includedirs {"../" .. ping_dir .. "/Source"}
   includedirs {vulkan_sdk_path .. "/Include"}
   includedirs {"../" .. glfw_dir .. "/include"}
   includedirs {"../" .. spdlog_dir .. "/include"}
   includedirs {"../" .. imgui_dir}
   
   filter "action:vs*"
       defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
       characterset ("Unicode")

   filter "system:windows"
       systemversion "latest"
       defines { }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"