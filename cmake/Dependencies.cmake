set(MUPFEL_NVRHI_COMMIT         "cca66aeb084429c880d205e508f5276b6ac13c38")
set(MUPFEL_VK_BOOTSTRAP_VERSION "1.4.357")
set(MUPFEL_IMGUI_COMMIT         "6029ee3789a2b7898f6423ec0c88cc4e5425f5a9")

set(MUPFEL_VENDOR_SOURCES "${MUPFEL_ROOT}/Vendor/Sources")

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
    SINGLE_FILES "https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp"
        "https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json_fwd.hpp")

mupfel_declare_dep(glm
    RELPATH "Vendor/Sources/glm-1.0.1"
    URL     "https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip")

mupfel_declare_dep(nvrhi
    RELPATH "Vendor/Sources/NVRHI-${MUPFEL_NVRHI_COMMIT}"
    URL     "https://github.com/NVIDIA-RTX/NVRHI/archive/${MUPFEL_NVRHI_COMMIT}.zip")

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
