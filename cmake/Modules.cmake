# cmake/Modules.cmake
#
# This file defines the modules of mupfel. Currently there exist these:
# * core (the mupfel engine, library only)
# * app (a demo application aiming to show all of mupfel's functionality)
# * tests (a testframework based on catch2)
#
#   cmake --preset windows                             -- everything
#   cmake --preset windows -D MUPFEL_MODULES=app       -- engine + game only
#   cmake --preset windows -D MUPFEL_MODULES=tests     -- engine + unit tests
#
# Names are the keys of the module list below, or "all"; anything else aborts configuration with the
# valid list rather than silently producing a build missing a target. Order and whitespace don't
# matter and the parse is case-insensitive ("App, CORE" is fine).
#
# The selection is closed over each module's requirements, so asking for a module always brings in
# what it links against. App and Tests link Core, which means **Core is part of every possible
# selection** -- and so is every vendored library Core links (nvrhi, nvrhi_vk, vk-bootstrap, spdlog,
# imgui, box2d), which is why Vendor/CMakeLists.txt only bothers gating catch2.
#
# Adding a module later is one entry below plus its CMakeLists.txt; nothing else in the build reads
# the option directly.

set(MUPFEL_MODULES "all" CACHE STRING "Comma-separated modules to generate: all, core, app, tests")

# Fixed order for subdirectory emission, the startup-project fallback and error messages.
set(MUPFEL_MODULE_ORDER core app tests)

# <module>_DIR       -- the subdirectory holding its CMakeLists.txt.
# <module>_TARGET    -- the target name it defines.
# <module>_REQUIRES  -- modules that must be in the build too, because this one links them.
# <module>_STARTABLE -- can be the IDE's startup project. Core can't: it's a static lib.
set(MUPFEL_core_DIR       "Core")
set(MUPFEL_core_TARGET    "Core")
set(MUPFEL_core_REQUIRES  "")
set(MUPFEL_core_STARTABLE OFF)

set(MUPFEL_app_DIR       "App")
set(MUPFEL_app_TARGET    "App")
set(MUPFEL_app_REQUIRES  "core")
set(MUPFEL_app_STARTABLE ON)

set(MUPFEL_tests_DIR       "Tests")
set(MUPFEL_tests_TARGET    "Tests")
set(MUPFEL_tests_REQUIRES  "core")
set(MUPFEL_tests_STARTABLE ON)

# Selects `name` and, recursively, everything it requires. CMake functions have their own variable
# scope and no recursion-friendly way to return a set, so the accumulator lives in the cache.
function(_mupfel_select_module name)
    if(MUPFEL_MODULE_SELECTED_${name})
        return()
    endif()

    if(NOT DEFINED MUPFEL_${name}_TARGET)
        string(REPLACE ";" ", " valid "${MUPFEL_MODULE_ORDER}")
        message(FATAL_ERROR
            "Unknown module '${name}' passed to MUPFEL_MODULES. Valid values: all, ${valid}")
    endif()

    set(MUPFEL_MODULE_SELECTED_${name} ON CACHE INTERNAL "")
    foreach(required IN LISTS MUPFEL_${name}_REQUIRES)
        _mupfel_select_module(${required})
    endforeach()
endfunction()

# Cleared on every configure so that re-running with a narrower MUPFEL_MODULES actually narrows the
# build instead of unioning with the previous run's cached selection.
foreach(module IN LISTS MUPFEL_MODULE_ORDER)
    unset(MUPFEL_MODULE_SELECTED_${module} CACHE)
endforeach()

string(REPLACE "," ";" _mupfel_requested "${MUPFEL_MODULES}")
foreach(name IN LISTS _mupfel_requested)
    string(STRIP "${name}" name)
    string(TOLOWER "${name}" name)

    if(name STREQUAL "all")
        foreach(module IN LISTS MUPFEL_MODULE_ORDER)
            _mupfel_select_module(${module})
        endforeach()
    elseif(NOT name STREQUAL "")
        _mupfel_select_module(${name})
    endif()
endforeach()

# The selected modules, in MUPFEL_MODULE_ORDER order, plus the per-module booleans the rest of the
# build tests (MUPFEL_MODULE_TESTS gates catch2, for instance).
set(MUPFEL_SELECTED_MODULES "")
foreach(module IN LISTS MUPFEL_MODULE_ORDER)
    string(TOUPPER "${module}" upper)
    if(MUPFEL_MODULE_SELECTED_${module})
        list(APPEND MUPFEL_SELECTED_MODULES ${module})
        set(MUPFEL_MODULE_${upper} ON)
    else()
        set(MUPFEL_MODULE_${upper} OFF)
    endif()
endforeach()

if(NOT MUPFEL_SELECTED_MODULES)
    string(REPLACE ";" ", " valid "${MUPFEL_MODULE_ORDER}")
    message(FATAL_ERROR "MUPFEL_MODULES selected nothing. Valid values: all, ${valid}")
endif()

# Project the IDE should start on: App when it's in (the historical default), otherwise the first
# startable module. Stays empty for a Core-only build, which has nothing runnable in it -- and
# VS_STARTUP_PROJECT must then not name a target that doesn't exist.
set(MUPFEL_START_PROJECT "")
foreach(module IN LISTS MUPFEL_SELECTED_MODULES)
    if(MUPFEL_${module}_STARTABLE AND NOT MUPFEL_START_PROJECT)
        set(MUPFEL_START_PROJECT "${MUPFEL_${module}_TARGET}")
    endif()
endforeach()

# Echoed because MUPFEL_MODULES silently changes what the build contains; without this a stale entry
# in someone's CMakeCache looks like targets vanishing on their own.
string(REPLACE ";" ", " _mupfel_selected_pretty "${MUPFEL_SELECTED_MODULES}")
message(STATUS "Modules: ${_mupfel_selected_pretty}")
