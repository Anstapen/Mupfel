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
    glm = {
        relpath = "Vendor/Sources/glm-master",
        url     = "https://github.com/g-truc/glm/archive/refs/heads/master.zip",
    },
    ping = {
        relpath = "Vendor/Sources/vulkan_starter-main/Ping",
        url     = "https://github.com/Anstapen/vulkan_starter/archive/refs/heads/main.zip",
    },
    nanobench = {
        relpath     = "Vendor/Sources/nanobench",
        url         = "https://raw.githubusercontent.com/martinus/nanobench/v4.3.11/src/include/nanobench.h",
        single_file = "nanobench.h", -- header-only microbenchmark framework, used by the Benchmarks project
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

-- Downloads dep.url into Vendor/Sources/, extracting it if it's a zip archive. No-ops if the
-- dependency is already present (its directory, or for single_file deps the file itself, exists).
function fetch_dependency(name)
    local dep = Deps[name]
    local marker = dep.single_file and (dep.relpath .. "/" .. dep.single_file) or dep.relpath

    if os.isdir(marker) or os.isfile(marker) then
        return
    end

    print("Fetching " .. name .. " from " .. dep.url)
    local sources_dir = "Vendor/Sources"
    if not os.isdir(sources_dir) then
        os.mkdir(sources_dir)
    end

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
    fetch_dependency("glm")
    fetch_dependency("nanobench")
    if os.target() == "windows" then
        fetch_dependency("glfw")
    end
end

build_externals()
