-- Dependencies.lua
--
-- Single source of truth for every vendored third-party dependency Mupfel builds against: where its
-- source lives once fetched (Deps.<name>.relpath, relative to the repo root) and where to download it
-- from if missing. Vendor/Build-Vendor.lua, Core/Build-Core.lua and App/Build-App.lua all resolve
-- include/lib paths through DepPath() below instead of hardcoding directory names or version strings,
-- so bumping a version only means editing the table below.

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
    ping = {
        relpath = "Vendor/Sources/vulkan_starter-main/Ping",
        url     = "https://github.com/Anstapen/vulkan_starter/archive/refs/heads/main.zip",
    },
    box2d = {
        relpath = "Vendor/Sources/box2d-3.1.1",
        url     = "https://github.com/erincatto/box2d/archive/refs/tags/v3.1.1.zip",
    },
    nanobench = {
        relpath     = "Vendor/Sources/nanobench",
        url         = "https://raw.githubusercontent.com/martinus/nanobench/v4.3.11/src/include/nanobench.h",
        single_file = "nanobench.h", -- header-only microbenchmark framework, used by the Benchmarks project
    },
    -- Catch2 v3 is normally consumed through CMake, which generates catch_user_config.hpp from a .in
    -- template before anything can compile. Upstream also publishes an "amalgamated" build of each
    -- release - one header plus one .cpp, with that config already baked in - so we take those two
    -- files instead and build them as an ordinary static lib (see Vendor/Build-Vendor.lua). Used by
    -- the Tests project.
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
    fetch_dependency("ping")
    fetch_dependency("spdlog")
    fetch_dependency("stb")
    fetch_dependency("imgui")
    fetch_dependency("nanobench")
    fetch_dependency("catch2")
    fetch_dependency("box2d")
    if os.target() == "windows" then
        fetch_dependency("glfw")
    end
end

build_externals()
