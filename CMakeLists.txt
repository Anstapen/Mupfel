# Mupfel
#
# Build layout (see BUILD.md for the full dependency-graph writeup):
#   Vendor/CMakeLists.txt   third-party libraries built from vendored source
#   Core/CMakeLists.txt     Mupfel's own engine static library, "Core"
#   App/CMakeLists.txt      the executable, and the IDE's startup project
#   Tests/CMakeLists.txt    Catch2 unit tests, links Core
#
# Everything below Vendor is conditional: -D MUPFEL_MODULES=core,app configures a build with just
# those two, and the default remains all three. The option, the module table and the dependency
# closure that keeps a selection buildable all live in cmake/Modules.cmake.
#
# CMake 4.0 is the floor because the build relies on the MSVC_RUNTIME_CHECKS target property (3.31)
# for box2d, and because the "Visual Studio 18 2026" generator the Windows preset names arrived in the
# 4.x series.

cmake_minimum_required(VERSION 4.0)

project(Mupfel LANGUAGES C CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "In-source builds are not supported. Configure into a subdirectory, e.g. "
        "`cmake --preset windows`, which builds into Build/windows.")
endif()

set(MUPFEL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${MUPFEL_ROOT}/cmake")

# Emitted for Ninja and Makefile generators only; this is what CLion, VS Code and clangd read to get
# per-file include paths and defines. The Visual Studio generator ignores it and needs no equivalent.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Mirrors the Premake solution's groups (Vendor, Engine, Tests).
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --------------------------------------------------------------------------------------------------
# Configurations
#
# Two: Debug and Release. Both are configurations CMake already knows, so neither needs its flag
# variables defined by hand.
#
# The Premake build also had a `Dist` (Release code generation, symbols stripped). It was dropped
# because nothing used it: no source file tested the `DIST` define and CI only ever built Debug and
# Release. Reinstating it is more than adding a name to the list below -- a custom configuration needs
# CMAKE_{C,CXX}_FLAGS_DIST, the four linker-flag variants and CMAKE_MAP_IMPORTED_CONFIG_DIST all
# defined explicitly, because an undefined one silently compiles unoptimized with no NDEBUG. See
# BUILD.md for what that looked like.
# --------------------------------------------------------------------------------------------------

get_property(_mupfel_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_mupfel_multi_config)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
else()
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
    endif()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

# Premake's Release is `optimize "Speed"` *with* symbols; CMake's carries none.
#
# Two halves, and missing either one gives a Release you cannot debug:
#   compiler  the debug information format set below, and -g on GCC.
#   linker    /DEBUG is what produces the .pdb for the executable itself. CMake adds it only for
#             Debug and RelWithDebInfo; our Release is neither, so without this App.exe and Tests.exe
#             ship with no symbols at all -- while the per-target .pdb files sit in the output
#             directory looking like the job was done.
#
# /OPT:REF and /OPT:ICF come back explicitly because the linker turns both *off* by default as soon as
# /DEBUG is present. Without them Release would silently stop folding identical COMDATs and stripping
# unreferenced ones, which is a size and layout regression, not just a symbols change. Verified: with
# them, App.exe is byte-for-byte the same size as a Release built without /DEBUG at all.
#
# Debug states /DEBUG too, rather than trusting CMake's default for it. That default is a *cache*
# entry, so it is whatever the first configure of a build directory happened to produce and it never
# updates afterwards: a directory first configured with a Clang toolchain (or with LDFLAGS set) keeps
# empty CMAKE_EXE_LINKER_FLAGS_DEBUG forever, even after switching to cl. That produced a Debug
# App.exe with an empty Debug Directory -- full -Z7 debug info in the objects, no PDB written, no
# breakpoints bindable -- while the Visual Studio generator's directory, configured by a different
# CMake, worked fine. Stating the flag here makes it independent of how a build directory was born.
if(MSVC)
    foreach(kind IN ITEMS EXE SHARED MODULE)
        string(APPEND CMAKE_${kind}_LINKER_FLAGS_DEBUG " /DEBUG")
        string(APPEND CMAKE_${kind}_LINKER_FLAGS_RELEASE " /DEBUG /OPT:REF /OPT:ICF")
    endforeach()
else()
    string(APPEND CMAKE_C_FLAGS_RELEASE " -g")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -g")
endif()

# staticruntime "off" in the Premake build.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# CMake's default gives debug information to Debug and RelWithDebInfo; the latter is not one of our
# configurations, so Release would carry none. Both of ours get it, matching Premake's per-configuration
# symbols setting. Must be set before any target is created.
#
# Embedded (/Z7), not ProgramDatabase (/Zi), and that choice is load-bearing rather than stylistic.
# With /Zi each static library writes a separate .pdb, and the linker then has to find it again when
# an executable consumes that library. It looks next to the .lib and in the consuming target's output
# directory -- and because every target here gets its own output directory, it finds neither:
#
#   Core.lib(Application.obj) : warning LNK4099: PDB 'Core.pdb' was not found ...;
#   linking object as if no debug information
#
# That is every Core object contributing no symbols to Tests.exe, announced as a warning nobody reads.
# /Z7 puts the debug information in the object files instead, so there is no intermediate .pdb to lose
# and the linker rolls everything into one complete .pdb per executable. It is also what the Premake
# ninja build used.
#
# Cost: the static libraries get considerably larger (Core.lib roughly doubles), since they now carry
# the debug information the .pdb used to hold. They are build artifacts under Binaries/, and the
# shipped executable is unaffected -- App.exe comes out byte-identical in size either way.
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")

# --------------------------------------------------------------------------------------------------
# Modules, dependencies, toolchain
# --------------------------------------------------------------------------------------------------

# Modules first: it parses MUPFEL_MODULES, and Dependencies skips fetching the sources whose module
# isn't in this build.
include(Modules)
include(Dependencies)
include(MupfelSettings)
include(VulkanSDK)

# The dependency sources have to be on disk before anything below can glob them or put them on an
# include path, and GLFW's interface target names paths inside the tree this populates.
mupfel_fetch_all()
include(GLFW)

# --------------------------------------------------------------------------------------------------
# Targets
# --------------------------------------------------------------------------------------------------

# Must be called here, in the top-level directory, not in Tests/: enable_testing() is what makes CMake
# emit a CTestTestfile.cmake that descends into the subdirectories. Called from Tests/ alone, the root
# build directory gets no test list and `ctest` reports "No tests were found".
enable_testing()

# Vendor is unconditional: every possible module selection contains Core (see cmake/Modules.cmake),
# and Core links nvrhi, nvrhi_vk, vk-bootstrap, spdlog, imgui and box2d. catch2 is the one vendored
# target that isn't always needed, and it gates itself inside Vendor/CMakeLists.txt.
add_subdirectory(Vendor)

foreach(module IN LISTS MUPFEL_SELECTED_MODULES)
    add_subdirectory("${MUPFEL_${module}_DIR}")
endforeach()

# Left unset for a Core-only build: a startup project naming a target that was never created confuses
# the IDE.
if(MUPFEL_START_PROJECT)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "${MUPFEL_START_PROJECT}")
endif()
