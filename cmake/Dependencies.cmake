# cmake/Dependencies.cmake
#
# Single source of truth for every vendored third-party dependency Mupfel builds against: where its
# source lives once fetched (relative to the repo root) and where to download it from if missing.
# Which of them actually get fetched depends on the module selection -- see mupfel_fetch_all() at the
# bottom, and cmake/Modules.cmake.
#
# Vendor/CMakeLists.txt, Core/CMakeLists.txt and the rest resolve include paths through the
# MUPFEL_DEP_<name>_DIR variables defined here instead of hardcoding directory names or version
# strings, so bumping a version only means editing the declarations below.
#
# Upstream CMakeLists.txt files are deliberately NOT used. Every vendored library is rebuilt as one of
# our own targets in Vendor/CMakeLists.txt, for the reasons BUILD.md gives under "NVRHI -- ported from
# CMake rather than built by it": upstream's builds generate nothing we need, and adopting them would
# drag in their option sets, their install rules and their own dependency resolution. This is why the
# fetch below is a plain download-and-extract rather than FetchContent_MakeAvailable() -- we want the
# sources on disk, not the projects in our build graph.

set(MUPFEL_NVRHI_COMMIT         "cca66aeb084429c880d205e508f5276b6ac13c38") # NVRHI main @ 2026-08-18; the repo publishes no tags
set(MUPFEL_VK_BOOTSTRAP_VERSION "1.4.357") # keep <= the Vulkan SDK floor asserted in the root CMakeLists.txt
set(MUPFEL_IMGUI_COMMIT         "6029ee3789a2b7898f6423ec0c88cc4e5425f5a9") # imgui docking branch, pinned for docking support

set(MUPFEL_VENDOR_SOURCES "${MUPFEL_ROOT}/Vendor/Sources")

# Registers a dependency. Exactly one of the three shapes applies, which decides how it is fetched:
#   SINGLE_FILES  a list of full file URLs, for deps distributed as a handful of loose files rather
#                 than an archive (Catch2's amalgamated header + source). Each file is checked and
#                 fetched on its own, so an interrupted run resumes rather than leaving the directory
#                 half-populated (its mere existence would look "present" to the checks below).
#   SINGLE_FILE   one loose file, URL pointing straight at it (json.hpp, stb_image.h).
#   otherwise     URL is a zip archive, extracted into Vendor/Sources/.
#
# Stored in the cache because these are read from every subdirectory scope in the build.
function(mupfel_declare_dep name)
    cmake_parse_arguments(D "WINDOWS_ONLY" "RELPATH;URL;SINGLE_FILE" "SINGLE_FILES" ${ARGN})

    set(MUPFEL_DEP_${name}_DIR          "${MUPFEL_ROOT}/${D_RELPATH}" CACHE INTERNAL "")
    set(MUPFEL_DEP_${name}_URL          "${D_URL}"                    CACHE INTERNAL "")
    set(MUPFEL_DEP_${name}_SINGLE_FILE  "${D_SINGLE_FILE}"            CACHE INTERNAL "")
    set(MUPFEL_DEP_${name}_SINGLE_FILES "${D_SINGLE_FILES}"           CACHE INTERNAL "")
    set(MUPFEL_DEP_${name}_WINDOWS_ONLY "${D_WINDOWS_ONLY}"           CACHE INTERNAL "")
endfunction()

mupfel_declare_dep(spdlog
    RELPATH "Vendor/Sources/spdlog-1.17.0"
    URL     "https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.zip")

mupfel_declare_dep(imgui
    RELPATH "Vendor/Sources/imgui-${MUPFEL_IMGUI_COMMIT}"
    URL     "https://github.com/ocornut/imgui/archive/${MUPFEL_IMGUI_COMMIT}.zip")

# Prebuilt Windows binaries; Linux builds link the system-installed GLFW instead.
mupfel_declare_dep(glfw
    RELPATH "Vendor/Sources/glfw-3.4.bin.WIN64"
    URL     "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip"
    WINDOWS_ONLY)

mupfel_declare_dep(stb
    RELPATH     "Vendor/Sources/stb"
    URL         "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
    SINGLE_FILE "stb_image.h")

mupfel_declare_dep(nlohmann
    RELPATH     "Vendor/Sources/nlohmann"
    URL         "https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp"
    SINGLE_FILE "json.hpp")

# Header-only, and vendored rather than taken from the Vulkan SDK: the LunarG *Windows* installer
# happens to ship a copy under its own Include/, which is what Core used to resolve <glm/glm.hpp>
# against. That quietly made a full SDK install a requirement of the build on every platform.
# The archive root already contains the glm/ directory, so the include path is the dep dir itself.
mupfel_declare_dep(glm
    RELPATH "Vendor/Sources/glm-1.0.1"
    URL     "https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip")

# NVRHI (NVIDIA Rendering Hardware Interface), MIT. Pinned to a commit rather than a tag because the
# repository publishes none.
mupfel_declare_dep(nvrhi
    RELPATH "Vendor/Sources/NVRHI-${MUPFEL_NVRHI_COMMIT}"
    URL     "https://github.com/NVIDIA-RTX/NVRHI/archive/${MUPFEL_NVRHI_COMMIT}.zip")

# vk-bootstrap, MIT. NVRHI deliberately does not create instances, devices, queues or swapchains --
# that is the application's job -- and this is the smallest library that does all four. One .cpp, no
# defines, and no link-time dependency on the Vulkan loader (it dlopens it at runtime, which is why
# Core links `dl` on Linux).
#
# The tag tracks the Vulkan header version the sources were generated against, so this pin and the SDK
# floor asserted in the root CMakeLists.txt have to move together.
mupfel_declare_dep(vk_bootstrap
    RELPATH "Vendor/Sources/vk-bootstrap-${MUPFEL_VK_BOOTSTRAP_VERSION}"
    URL     "https://github.com/charles-lunarg/vk-bootstrap/archive/refs/tags/v${MUPFEL_VK_BOOTSTRAP_VERSION}.zip")

mupfel_declare_dep(box2d
    RELPATH "Vendor/Sources/box2d-3.1.1"
    URL     "https://github.com/erincatto/box2d/archive/refs/tags/v3.1.1.zip")

mupfel_declare_dep(catch2
    RELPATH      "Vendor/Sources/catch2-3.15.3"
    SINGLE_FILES "https://github.com/catchorg/Catch2/releases/download/v3.15.3/catch_amalgamated.hpp"
                 "https://github.com/catchorg/Catch2/releases/download/v3.15.3/catch_amalgamated.cpp")

# Downloads one file, aborting generation on any transport or HTTP error. file(DOWNLOAD) writes a
# (possibly empty) file even when the request fails, so the failed download is removed rather than
# left behind to satisfy the "already present" checks on the next run.
function(mupfel_download url destination)
    message(STATUS "Fetching ${url}")
    file(DOWNLOAD "${url}" "${destination}" STATUS status LOG log SHOW_PROGRESS TLS_VERIFY ON)

    list(GET status 0 code)
    if(NOT code EQUAL 0)
        list(GET status 1 reason)
        file(REMOVE "${destination}")
        message(FATAL_ERROR "Failed to download ${url}: ${reason}\n${log}")
    endif()
endfunction()

# Downloads a dependency into Vendor/Sources/ if it isn't already there.
function(mupfel_fetch_dependency name)
    set(dir "${MUPFEL_DEP_${name}_DIR}")

    if(MUPFEL_DEP_${name}_SINGLE_FILES)
        foreach(url IN LISTS MUPFEL_DEP_${name}_SINGLE_FILES)
            string(REGEX MATCH "[^/]+$" filename "${url}")
            if(NOT EXISTS "${dir}/${filename}")
                mupfel_download("${url}" "${dir}/${filename}")
            endif()
        endforeach()
        return()
    endif()

    if(MUPFEL_DEP_${name}_SINGLE_FILE)
        set(marker "${dir}/${MUPFEL_DEP_${name}_SINGLE_FILE}")
        if(NOT EXISTS "${marker}")
            mupfel_download("${MUPFEL_DEP_${name}_URL}" "${marker}")
        endif()
        return()
    endif()

    if(EXISTS "${dir}")
        return()
    endif()

    # Extracted into Vendor/Sources/ rather than into `dir`: every archive's root is already the
    # versioned directory name that RELPATH names, so extracting one level up lands it exactly there.
    set(archive "${MUPFEL_VENDOR_SOURCES}/${name}.zip")
    file(MAKE_DIRECTORY "${MUPFEL_VENDOR_SOURCES}")
    mupfel_download("${MUPFEL_DEP_${name}_URL}" "${archive}")

    message(STATUS "Unzipping to ${MUPFEL_VENDOR_SOURCES}")
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${MUPFEL_VENDOR_SOURCES}")
    file(REMOVE "${archive}")

    if(NOT EXISTS "${dir}")
        message(FATAL_ERROR
            "${name} extracted, but ${dir} does not exist. The archive's root directory name probably "
            "changed upstream; update its RELPATH in cmake/Dependencies.cmake.")
    endif()
endfunction()

function(mupfel_fetch_all)
    message(STATUS "Checking external dependencies...")

    foreach(dep IN ITEMS nlohmann glm nvrhi vk_bootstrap spdlog stb imgui box2d)
        mupfel_fetch_dependency(${dep})
    endforeach()

    if(MUPFEL_MODULE_TESTS)
        mupfel_fetch_dependency(catch2)
    endif()

    if(WIN32)
        mupfel_fetch_dependency(glfw)
    endif()
endfunction()
