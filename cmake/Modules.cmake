set(MUPFEL_MODULES "all" CACHE STRING "Comma-separated modules to generate: all, core, app, tests")

set(MUPFEL_MODULE_ORDER core app tests)

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

set(MUPFEL_START_PROJECT "")
foreach(module IN LISTS MUPFEL_SELECTED_MODULES)
    if(MUPFEL_${module}_STARTABLE AND NOT MUPFEL_START_PROJECT)
        set(MUPFEL_START_PROJECT "${MUPFEL_${module}_TARGET}")
    endif()
endforeach()

string(REPLACE ";" ", " _mupfel_selected_pretty "${MUPFEL_SELECTED_MODULES}")
message(STATUS "Modules: ${_mupfel_selected_pretty}")
