project "App"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp", "Shaders/**.glsl" }

   includedirs
   {
      "Source",

	  -- Include Core
	  "../Core/Source"
   }

   links
   {
      "Core"
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")
   includedirs {"../Vendor/Sources/nlohmann"}
   includedirs {"../Vendor/Sources/glm-master/glm"}
   includedirs {"../Vendor/Sources/vulkan_starter-main/Ping/Source"}
   includedirs {"../Vendor/Sources/spdlog-1.17.0/include"}
   includedirs {vulkan_sdk_path .. "/Include"}
  

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }

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