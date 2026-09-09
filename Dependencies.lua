-- Dependencies.lua
--
-- Single source of truth for every vendored third-party dependency Mupfel builds against: where its
-- source lives once fetched (Deps.<name>.relpath, relative to the repo root) and where to download it
-- from if missing. Which of them actually get fetched depends on the --modules selection -- see
-- build_externals() at the bottom, and Modules.lua. Vendor/Build-Vendor.lua, Core/Build-Core.lua and App/Build-App.lua all resolve
-- include/lib paths through DepPath() below instead of hardcoding directory names or version strings,
-- so bumping a version only means editing the table below.

local nvrhi_commit        = "cca66aeb084429c880d205e508f5276b6ac13c38" -- NVRHI main @ 2026-08-18; the repo publishes no tags
local vk_bootstrap_version = "1.4.357" -- keep <= the Vulkan SDK floor asserted in Build.lua

local imgui_commit = "6029ee3789a2b7898f6423ec0c88cc4e5425f5a9" -- imgui docking branch, pinned for docking support

Deps = {
    spdlog = {
        relpath = "Vendor/Sources/spdlog-1.17.0",
        url     = "https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.zip",
    },
    imgui = {
        relpath = "Vendor/Sources/imgui-" .. imgui_commit,
        url     = "https://github.com/ocornut/imgui/archive/" .. imgui_commit .. ".zip",
    },
    glfw = {
        relpath      = "Vendor/Sources/glfw-3.4.bin.WIN64",
        url          = "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip",
        windows_only = true, -- prebuilt Windows binaries; Linux builds link the system-installed GLFW instead
    },
    stb = {
        relpath     = "Vendor/Sources/stb",
        url         = "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h",
        single_file = "stb_image.h",
    },
    nlohmann = {
        relpath     = "Vendor/Sources/nlohmann",
        url         = "https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp",
        single_file = "json.hpp",
    },
    -- Header-only, and vendored rather than taken from the Vulkan SDK: the LunarG *Windows* installer
    -- happens to ship a copy under its own Include/, which is what Core used to resolve <glm/glm.hpp>
    -- against. That quietly made a full SDK install a requirement of the build on every platform.
    -- The archive root already contains the glm/ directory, so the include path is DepPath("glm").
    glm = {
        relpath = "Vendor/Sources/glm-1.0.1",
        url     = "https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip",
    },
    -- NVRHI (NVIDIA Rendering Hardware Interface), MIT. Pinned to a commit rather than a tag because
    -- the repository publishes none. Its own CMake build is not used at all -- Vendor/Build-Vendor.lua
    -- reimplements the two targets we need as Premake static libs (see BUILD.md, "NVRHI -- ported from
    -- CMake rather than built by it").
    nvrhi = {
        relpath = "Vendor/Sources/NVRHI-" .. nvrhi_commit,
        url     = "https://github.com/NVIDIA-RTX/NVRHI/archive/" .. nvrhi_commit .. ".zip",
    },
    -- vk-bootstrap, MIT. NVRHI deliberately does not create instances, devices, queues or swapchains
    -- -- that is the application's job -- and this is the smallest library that does all four. One
    -- .cpp, no defines, and no link-time dependency on the Vulkan loader (it dlopens it at runtime,
    -- which is why Core links `dl` on Linux).
    --
    -- The tag tracks the Vulkan header version the sources were generated against, so this pin and the
    -- SDK floor asserted in Build.lua have to move together.
    vk_bootstrap = {
        relpath = "Vendor/Sources/vk-bootstrap-" .. vk_bootstrap_version,
        url     = "https://github.com/charles-lunarg/vk-bootstrap/archive/refs/tags/v"
                  .. vk_bootstrap_version .. ".zip",
    },
    box2d = {
        relpath = "Vendor/Sources/box2d-3.1.1",
        url     = "https://github.com/erincatto/box2d/archive/refs/tags/v3.1.1.zip",
    },
    catch2 = {
        relpath      = "Vendor/Sources/catch2-3.15.3",
        single_files = {
            "https://github.com/catchorg/Catch2/releases/download/v3.15.3/catch_amalgamated.hpp",
            "https://github.com/catchorg/Catch2/releases/download/v3.15.3/catch_amalgamated.cpp",
        },
    },
}

-- Resolves a path inside a dependency's source tree, anchored to the workspace root via the
-- %{wks.location} token so it stays correct from any project script regardless of nesting depth
-- (e.g. called from both Vendor/Build-Vendor.lua and App/Build-App.lua).
function DepPath(name, subpath)
    local p = "%{wks.location}/" .. Deps[name].relpath
    if subpath then
        p = p .. "/" .. subpath
    end
    return p
end

function download_progress(total, current)
    local ratio = math.min(math.max(current / total, 0), 1)
    print("Download progress (" .. math.floor(ratio * 100) .. "%/100%)")
end

-- Downloads a dependency into Vendor/Sources/ if it isn't already there. Which of the three shapes a
-- Deps entry takes decides how:
--   dep.single_files -- a list of full file URLs, for deps distributed as a handful of loose files
--                       rather than an archive (Catch2's amalgamated header + source). Each file is
--                       checked and fetched on its own, so an interrupted run resumes rather than
--                       leaving the directory half-populated (its mere existence would look "present"
--                       to the checks below).
--   dep.single_file  -- one loose file, dep.url pointing straight at it (json.hpp, stb_image.h).
--   otherwise        -- dep.url is a zip archive, extracted into Vendor/Sources/.
function fetch_dependency(name)
    local dep = Deps[name]
    local marker = dep.single_file and (dep.relpath .. "/" .. dep.single_file) or dep.relpath

    if not dep.single_files and (os.isdir(marker) or os.isfile(marker)) then
        return
    end

    local sources_dir = "Vendor/Sources"
    if not os.isdir(sources_dir) then
        os.mkdir(sources_dir)
    end

    if dep.single_files then
        for _, url in ipairs(dep.single_files) do
            local target = dep.relpath .. "/" .. url:match("[^/]+$")
            if not os.isfile(target) then
                print("Fetching " .. name .. " from " .. url)
                os.mkdir(dep.relpath)
                http.download(url, target, { progress = download_progress, headers = { "From: Premake", "Referer: Premake" } })
            end
        end
        return
    end

    print("Fetching " .. name .. " from " .. dep.url)

    if dep.single_file then
        os.mkdir(dep.relpath)
        http.download(dep.url, marker, { progress = download_progress, headers = { "From: Premake", "Referer: Premake" } })
        return
    end

    local archive = sources_dir .. "/" .. name .. ".zip"
    http.download(dep.url, archive, { progress = download_progress, headers = { "From: Premake", "Referer: Premake" } })
    print("Unzipping to " .. sources_dir)
    zip.extract(archive, sources_dir)
    os.remove(archive)
end

function build_externals()
    print("Checking external dependencies...")
    fetch_dependency("nlohmann")
    fetch_dependency("glm")
    fetch_dependency("nvrhi")
    fetch_dependency("vk_bootstrap")
    fetch_dependency("spdlog")
    fetch_dependency("stb")
    fetch_dependency("imgui")
    fetch_dependency("box2d")
    if ModuleSelected("tests") then
        fetch_dependency("catch2")
    end
    if os.target() == "windows" then
        fetch_dependency("glfw")
    end
end

build_externals()
