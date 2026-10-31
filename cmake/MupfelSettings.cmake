if(WIN32)
    set(MUPFEL_SYSTEM "windows")
elseif(APPLE)
    set(MUPFEL_SYSTEM "macosx")
else()
    set(MUPFEL_SYSTEM "linux")
endif()

set(MUPFEL_OUTPUT_DIR "${MUPFEL_ROOT}/Binaries/${MUPFEL_SYSTEM}-x86_64")

function(mupfel_apply_default_settings target)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${MUPFEL_OUTPUT_DIR}/$<CONFIG>/${target}"
        LIBRARY_OUTPUT_DIRECTORY "${MUPFEL_OUTPUT_DIR}/$<CONFIG>/${target}"
        ARCHIVE_OUTPUT_DIRECTORY "${MUPFEL_OUTPUT_DIR}/$<CONFIG>/${target}")

    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:RELEASE>)

    if(MSVC)
        target_compile_definitions(${target} PRIVATE
            UNICODE _UNICODE
            _WINSOCK_DEPRECATED_NO_WARNINGS
            _CRT_SECURE_NO_WARNINGS)

        target_compile_options(${target} PRIVATE
            /execution-charset:utf-8
            $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
            $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>)
    endif()
endfunction()

function(mupfel_apply_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4 /WX /w15038
            /external:W0)
        return()
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -Werror
            -Wreorder
            -Wunused-parameter
            -Wunused-variable
            -Wunused-function
            -Wunused-label
            -Wunused-value
            -Wuninitialized
            -Wempty-body
            -Wsign-compare
            -Wformat
            -Wbool-compare
            -Wunknown-pragmas
            -Winfinite-recursion
            -Wdelete-non-virtual-dtor
            -Wconversion
            -Wno-format-overflow
            -Wno-format-truncation
            -Wno-maybe-uninitialized)
    else()
        target_compile_options(${target} PRIVATE -Wall -Werror)
    endif()
endfunction()
