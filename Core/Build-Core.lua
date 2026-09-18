-- Core/Build-Core.lua
--
-- Builds the "Core" engine static library: Mupfel's own reusable engine code (ECS, physics/collision
-- pipeline, renderer, event system, etc.), see Core/Source. Everything it links against is vendored
-- (see Vendor/Build-Vendor.lua) or a system dependency (Vulkan SDK, GLFW prebuilt binary).
--
-- Dependency graph: Core -> nvrhi, nvrhi_vk, vk-bootstrap, spdlog, imgui, box2d (linked); glfw3,
-- vulkan (prebuilt/system, linked via libdirs); nlohmann json and stb_image headers only (entity
-- (de)serialization and image decoding, no link needed).
-- Core no longer links "Logger" directly -- Core/Include/Core/Logger.h is Mupfel's own spdlog wrapper.
--
-- Ping used to sit where nvrhi/nvrhi_vk/vk-bootstrap now are. NVRHI covers only resources, command
-- lists, pipelines and resource-state tracking; it explicitly does not create devices or swapchains
-- and has no window or ImGui integration, so three things Ping used to hand us now live in Core:
-- the vk-bootstrap glue that produces the handles nvrhi::vulkan::createDevice consumes, the
-- STB_IMAGE_IMPLEMENTATION translation unit, and the ImGui-on-NVRHI renderer backend. That last one
-- is why DepPath("imgui", "backends") is on the include path below and was not before.
--
-- Note: glm is vendored (Deps.glm in Dependencies.lua) and listed under externalincludedirs below.
-- It used to have no entry at all and resolved only because the LunarG *Windows* SDK happens to ship
-- a copy under its own Include directory -- which quietly made a full SDK install a requirement of
-- the build on every platform, glm having nothing to do with Vulkan. The path is DepPath("glm") and
-- not DepPath("glm", "glm"): the archive root already contains the glm/ directory that the five
-- users of it (Camera.cpp, ECSRenderer.cpp, IMRenderer.cpp, DebugLayer.cpp, Renderer/Quad.h) name
-- as <glm/glm.hpp>.
--
-- Headers are split across two roots. "Include" is the published surface, reachable from Mupfel.h and
-- the only root App puts on its include path (see App/Build-App.lua). "Source" holds engine internals
-- plus every .cpp, and App cannot see it -- that directory split IS the access control, since Premake
-- has no PUBLIC/PRIVATE include model.
--
-- Core itself needs both roots, and additionally each public subdirectory: existing code includes
-- siblings by bare name (#include "SubRenderer.h"), which stops resolving once the sibling moves to
-- the other root. Listing the subdirectories keeps the move free of source edits. All 41 public header
-- basenames are unique, so the flattened search is unambiguous; a future pass that path-qualifies
-- those includes ("Renderer/SubRenderer.h") lets the subdirectory entries be dropped again.

-- Everything Core.lib needs at link time. Core applies it to itself; under the ninja action, every
-- project that links Core applies it too, because that is the one generator where Core.lib does not
-- carry these on its own:
--   vs*    MSBuild's librarian merges <ProjectReference> libs and <Lib><AdditionalDependencies> into
--          Core.lib, so consumers need only `links { "Core" }`.
--   ninja  Premake's ar rule archives Core's own objects and ignores its links, and executables get
--          no transitive links, so App/Tests fail with unresolved nvrhi/ImGui/vk-bootstrap/Vulkan symbols.
-- Ends with `filter {}`, so call it where no filter is active.
function ApplyCoreLinkDependencies()
    libdirs
    {
        VulkanLibDir,
    }

    -- nvrhi_vk before nvrhi is deliberate for the GNU linker, which resolves left to right in one
    -- pass: the backend pulls symbols out of the core (state tracking, format info, the validation
    -- wrapper), never the other way round. MSVC does not care, but the same list feeds both.
    --
    -- vk-bootstrap needs no Vulkan import library of its own (it dlopens the loader), but Core still
    -- links VulkanLibName: glfwCreateWindowSurface and the surface/present calls in our own device
    -- layer are ordinary prototypes resolved at link time.
    links
    {
        "nvrhi_vk",
        "nvrhi",
        "vk-bootstrap",
        "spdlog",
        "imgui",
        VulkanLibName,
        "box2d",
    }

    -- GLFW is the one dependency that is not vendored the same way on both platforms, so it cannot be
    -- linked unconditionally: Windows uses the prebuilt binary fetched as Deps.glfw, Linux links the
    -- system libglfw.so from libglfw3-dev -- which is -lglfw, not -lglfw3.
    filter "system:windows"
        libdirs { DepPath("glfw", "lib-vc2022") }
        links   { "glfw3" }

    -- glfw3.lib's Win32 backend imports from gdi32 (CreateDIBSection, SwapBuffers, ...). MSBuild links
    -- it through its default CoreLibraryDependencies, but ninja's bare `cl ... /link` gets no such
    -- defaults.
    filter { "system:windows", "action:ninja" }
        links   { "gdi32" }

    -- dl is vk-bootstrap's: it does not link the Vulkan loader, it dlopen()s libvulkan.so.1 and
    -- resolves entry points through dlsym. Static libs carry no link dependencies of their own, so
    -- the requirement lands on Core. Windows needs no equivalent -- LoadLibrary lives in kernel32,
    -- which MSVC links by default.
    filter "system:not windows"
        links   { "glfw", "dl" }

    filter {}
end

project "Core"
    kind "StaticLib"
    ApplyDefaultProjectSettings()
    ApplyStrictWarnings()

    files { "Include/**.h", "Source/**.h", "Source/**.cpp" }

    includedirs
    {
        "Include",
        "Include/Core",
        "Include/ECS",
        "Include/ECS/Components",
        "Include/FS",
        "Include/Renderer",
        "Source",
    }

    -- Vendored/system headers, kept out of includedirs so ApplyStrictWarnings()'s -Werror / /WX can't
    -- fail the build over third-party code: these emit -isystem (clang/gcc) and <ExternalIncludePath>
    -- (MSVC), both compiled at the "Off" external warning level. See Build.lua.
    externalincludedirs
    {
        DepPath("nlohmann"),
        DepPath("glm"),
        DepPath("nvrhi", "include"),
        DepPath("vk_bootstrap", "src"),
        DepPath("stb"),
        VulkanIncludeDir,
        -- Windows-only prebuilt GLFW (Deps.glfw is windows_only). On Linux this path does not exist
        -- and the system headers installed by libglfw3-dev are found on the default search path.
        DepPath("glfw", "include"),
        DepPath("spdlog", "include"),
        DepPath("imgui"),
        DepPath("imgui", "backends"),
        DepPath("box2d", "include"),
    }

    -- NVRHI's src/vulkan/vulkan-backend.h does its own `#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1`
    -- before including <vulkan/vulkan.hpp>, and every Core TU that reaches vulkan.hpp has to agree with
    -- it. The macro decides what VULKAN_HPP_DEFAULT_DISPATCHER expands to (the dynamic loader vs. the
    -- static one) and therefore the default dispatch argument -- and the ABI of the inline entry
    -- points -- on every vk:: call. Setting it project-wide rather than per-TU means no Core TU can
    -- disagree with nvrhi_vk, and it is the only half of the rule a build system can enforce.
    --
    -- It is also what makes VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE in Renderer.cpp emit
    -- anything: without the define that macro expands to nothing at all, with no diagnostic. NVRHI
    -- defines that storage only in a shared build (NVRHI_SHARED_LIBRARY_BUILD), so in our static build
    -- Core owns both the storage and the three-step VULKAN_HPP_DEFAULT_DISPATCHER.init() in
    -- NVRHIContext::Init. See Reviews/NVRHI-Dispatcher.md.
    defines { "VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1" }

    -- Upstream marks this PUBLIC on nvrhi_vk, and it must match here: it decides whether
    -- <vulkan/vulkan.h> declares the Win32 surface entry points that our device layer calls, and a TU
    -- that disagrees with the one that compiled NVRHI sees a different Vulkan API. NOMINMAX comes
    -- along because that define is what drags windows.h in.
    filter "system:windows"
        defines { "VK_USE_PLATFORM_WIN32_KHR", "NOMINMAX" }

    filter {}

    ApplyCoreLinkDependencies()
