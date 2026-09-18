# cmake/MupfelSettings.cmake
#
# The shared per-target configuration every target calls into, so toolchain/output settings live in
# exactly one place instead of being copy-pasted per CMakeLists.txt. The Premake build spelled these
# as ApplyDefaultProjectSettings() and ApplyStrictWarnings() in Build.lua.

# Matches Premake's "%{cfg.system}-%{cfg.architecture}" so output paths (and therefore the CI scripts
# that run Binaries/windows-x86_64/<config>/Tests/Tests.exe) are unchanged by the migration.
if(WIN32)
    set(MUPFEL_SYSTEM "windows")
elseif(APPLE)
    set(MUPFEL_SYSTEM "macosx")
else()
    set(MUPFEL_SYSTEM "linux")
endif()

set(MUPFEL_OUTPUT_DIR "${MUPFEL_ROOT}/Binaries/${MUPFEL_SYSTEM}-x86_64")

# Applied to every target (vendor, engine, app).
#
# Unlike the Premake original this does not set the language standard: that is a per-target decision
# here (box2d is C17, the NVRHI/vk-bootstrap targets are C++17, ours are C++23), and CMake has no
# equivalent of Premake's "call the shared function, then override the dialect after it".
function(mupfel_apply_default_settings target)
    # $<CONFIG> is spelled explicitly so multi-config generators don't append the configuration a
    # second time. Static libs land beside executables, exactly where Premake's targetdir put them.
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${MUPFEL_OUTPUT_DIR}/$<CONFIG>/${target}"
        LIBRARY_OUTPUT_DIRECTORY "${MUPFEL_OUTPUT_DIR}/$<CONFIG>/${target}"
        ARCHIVE_OUTPUT_DIRECTORY "${MUPFEL_OUTPUT_DIR}/$<CONFIG>/${target}")

    # glm bakes the clip-space depth convention into ortho()/perspective() at compile time via
    # GLM_CONFIG_CLIP_CONTROL. Those are inline functions, so every TU must agree: if one TU sees this
    # define and another doesn't, the two differing bodies are an ODR violation and the linker
    # silently keeps whichever it saw first. Vulkan clips to 0 <= z <= w, so it must be [0,1] --
    # getting OpenGL's [-1,1] makes the whole scene fail the depth test and render black.
    # Set here rather than per-file so a stray `#include <glm/glm.hpp>` can't opt a TU out.
    target_compile_definitions(${target} PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)

    # CMake defines only NDEBUG (via its own Release flags), so the configuration names come from here.
    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:RELEASE>)

    if(MSVC)
        # characterset "Unicode" in the Premake build. It was scoped to the vs* action there, which
        # meant the ninja build silently compiled with the narrow CRT instead; a define set has no
        # business depending on which generator produced the build, so it applies to MSVC here.
        # catch2's DO_NOT_USE_WMAIN in Vendor/CMakeLists.txt is the counterpart of this.
        target_compile_definitions(${target} PRIVATE
            UNICODE _UNICODE
            _WINSOCK_DEPRECATED_NO_WARNINGS
            _CRT_SECURE_NO_WARNINGS)

        # spdlog's bundled fmt headers require an UTF-8 execution charset (their own headers are valid
        # UTF-8). We deliberately don't force /source-charset:utf-8 (which /utf-8 would also imply)
        # since some of our own headers aren't valid UTF-8 and MSVC would fail/warn (C4828) trying to
        # parse them as such.
        #
        # /EHsc and /Zc:__cplusplus are C++-only -- MSVC rejects both on a C compiland, which is what
        # box2d is. Premake needed an explicit removebuildoptions{} for that; the COMPILE_LANGUAGE
        # genex makes it automatic.
        target_compile_options(${target} PRIVATE
            /Zc:preprocessor
            /execution-charset:utf-8
            $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
            $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>)
    endif()
endfunction()

# Strict warnings, applied only to Mupfel's own compiled code (Core, Tests). Deliberately NOT part of
# mupfel_apply_default_settings(): every target calls that one, the vendored ones included, and
# third-party source keeps whatever warning level its authors settled on -- we don't fix their code,
# and a dependency bump must not be able to break our build.
#
# MSVC's /W4 is the reference level. GCC does not get -Wall: it gets the GCC counterparts of what /W4
# reports, and nothing else, so the Linux build cannot fail on code the Windows build accepts. The
# mapping, and the counterparts deliberately left out, are in BUILD.md ("Warning parity").
#
# The other half of "ignore vendored warnings" is include *paths*: a third-party header included from
# one of our .cpp files warns as if we had written it, so -Werror would fail our build over spdlog's
# or ImGui's code. The targets therefore pass every vendored path to
# target_include_directories(... SYSTEM ...), which emits -isystem (clang/gcc) and /external:I (MSVC),
# both compiled at the external warning level /external:W0 set below. Search order is unchanged: both
# are searched after the normal include dirs, which is exactly where the vendored entries already sat.
function(mupfel_apply_strict_warnings target)
    if(MSVC)
        # C5038 is MSVC's -Wreorder (member initializer list out of declaration order). GCC and Clang
        # have it in -Wall; MSVC keeps it off even at /W4, so without this only the Linux build catches
        # it. Emitted as /w15038, which /WX turns into an error.
        target_compile_options(${target} PRIVATE
            /W4 /WX /w15038
            /external:W0)
        return()
    endif()

    # No -Wall: the list below is the GCC counterpart of what /W4 reports, and nothing else. The
    # disabled ones come in implicitly -- -Wformat implies the snprintf/sprintf buffer-size analyses,
    # which MSVC has no equivalent of, and -Wuninitialized implies -Wmaybe-uninitialized, whose
    # optimizer-dependent results produce GCC-only false positives (see BUILD.md).
    #
    # Guarded on GNU because clang rejects some of these spellings (-Wbool-compare) as unknown, which
    # -Werror makes fatal.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -Werror
            -Wreorder                 # C5038
            -Wunused-parameter        # C4100
            -Wunused-variable         # C4101, C4189
            -Wunused-function         # C4505
            -Wunused-label            # C4102
            -Wunused-value            # C4553
            -Wuninitialized           # C4700
            -Wempty-body              # C4390
            -Wsign-compare            # C4018, C4389
            -Wformat                  # C4477
            -Wbool-compare            # C4806
            -Wunknown-pragmas         # C4068
            -Winfinite-recursion      # C4717
            -Wdelete-non-virtual-dtor # C5205
            -Wconversion              # C4244, C4267, C4305
            -Wno-format-overflow
            -Wno-format-truncation
            -Wno-maybe-uninitialized)
    else()
        target_compile_options(${target} PRIVATE -Wall -Werror)
    endif()
endfunction()
