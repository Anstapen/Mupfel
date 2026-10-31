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
    nvrhi_vk)

set_property(TARGET Tests PROPERTY FOLDER "Tests")

add_test(NAME Mupfel.Tests COMMAND Tests)
