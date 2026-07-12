glfw_dir = "Vendor/Sources/glfw-3.4.bin.WIN64"
spdlog_dir = "Vendor/Sources/spdlog-1.17.0"
stb_dir = "Vendor/Sources/stb"
imgui_commit = "6029ee3789a2b7898f6423ec0c88cc4e5425f5a9" -- imgui docking branch, pinned for docking support
imgui_dir = "Vendor/Sources/imgui-" .. imgui_commit
ping_dir = "Vendor/Sources/vulkan_starter-main/Ping"

project "spdlog"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    files { "../" .. spdlog_dir .. "/include/**.h", "../" .. spdlog_dir .. "/src/**.cpp" }

    includedirs { "../" .. spdlog_dir .. "/include" }

    targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
    objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

    defines { "SPDLOG_COMPILED_LIB" }

    filter "action:vs*"
        defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
        characterset ("Unicode")

    filter "system:windows"
        systemversion "latest"

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

    filter {}

-- Split out of Ping's own files{} glob so it's a standalone leaf dependency: both `Ping` (used
-- internally by VKManager etc.) and `Core` link it directly, without either needing the other.
project "Logger"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    files { "../" .. ping_dir .. "/Source/Logger/**.h", "../" .. ping_dir .. "/Source/Logger/**.cpp" }

    includedirs
    {
        "../" .. ping_dir .. "/Source",
        "../" .. spdlog_dir .. "/include"
    }

    targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
    objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

    filter "action:vs*"
        defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
        characterset ("Unicode")

    filter "system:windows"
        systemversion "latest"

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

    filter {}

project "imgui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    files
    {
        "../" .. imgui_dir .. "/*.h",
        "../" .. imgui_dir .. "/*.cpp",
        "../" .. imgui_dir .. "/backends/imgui_impl_glfw.h",
        "../" .. imgui_dir .. "/backends/imgui_impl_glfw.cpp"
    }

    includedirs
    {
        "../" .. imgui_dir,
        "../" .. imgui_dir .. "/backends",
        "../" .. glfw_dir .. "/include"
    }

    targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
    objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

    filter "action:vs*"
        defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
        characterset ("Unicode")

    filter "system:windows"
        systemversion "latest"

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

    filter {}

project "Ping"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    files { "../" .. ping_dir .. "/Source/**.h", "../" .. ping_dir .. "/Source/**.cpp" }
    -- Logger is its own project (see above) so it can be linked independently of the rest of Ping.
    removefiles { "../" .. ping_dir .. "/Source/Logger/**.h", "../" .. ping_dir .. "/Source/Logger/**.cpp" }

    includedirs
    {
        "../" .. ping_dir .. "/Source",
        vulkan_sdk_path .. "/Include",
        "../" .. glfw_dir .. "/include",
        "../" .. spdlog_dir .. "/include",
        "../" .. stb_dir,
        "../" .. imgui_dir,
        "../" .. imgui_dir .. "/backends"
    }

    targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
    objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

    links { "Logger" }

    filter "action:vs*"
        defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
        characterset ("Unicode")

    filter "system:windows"
        systemversion "latest"

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

    filter {}
