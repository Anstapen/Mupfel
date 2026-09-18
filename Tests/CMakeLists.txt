# Tests/CMakeLists.txt
#
# Builds the "Tests" executable: unit tests for the engine, built on Catch2 v3 (see the catch2 entry in
# cmake/Dependencies.cmake and the catch2 target in Vendor/CMakeLists.txt).
#
# Two source roots, both globbed and both on the include path, so adding a file to either needs no edit
# here -- and, unlike the Premake build, no manual regeneration either, because CONFIGURE_DEPENDS makes
# the build re-run the glob:
#   Source/  the tests themselves (Test_*.cpp)
#   Common/  test-only helpers shared between them (Random.h), deliberately kept out of Core since
#            nothing the engine ships depends on them
#
# Unlike App, the tests are white-box: they exercise engine internals (Core/ResourceManager.h) and so
# get Core's private header root as well as its published one. That is also why they link nvrhi_vk and
# Mupfel::glm directly -- an internal renderer header reachable from Core/Source names nvrhi types and
# glm types, and Core keeps both PRIVATE, so linking Core alone does not put them on the include path.
# In the Premake build this was a second copy of Core's externalincludedirs list, maintained by hand
# and already drifting: it listed stb, imgui and GLFW, which no test has ever needed.

file(GLOB_RECURSE tests_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Common/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Common/*.h")

add_executable(Tests ${tests_sources})
mupfel_apply_default_settings(Tests)
mupfel_apply_strict_warnings(Tests)

target_compile_features(Tests PRIVATE cxx_std_23)

target_include_directories(Tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    "${CMAKE_CURRENT_SOURCE_DIR}/Common"
    "${MUPFEL_ROOT}/Core/Source")

target_link_libraries(Tests PRIVATE
    Core
    catch2
    nvrhi_vk
    Mupfel::glm)

set_property(TARGET Tests PROPERTY FOLDER "Tests")

# Registers the Catch2 binary as a single CTest test, so `ctest --preset windows-debug` runs the suite.
# The CI workflows invoke the executable directly, as they did before; this is for local use and for
# the IDE test runners that discover CTest. enable_testing() is called in the root CMakeLists.txt --
# calling it here instead leaves the root build directory with no test list at all.
add_test(NAME Mupfel.Tests COMMAND Tests)
