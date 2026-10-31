file(GLOB_RECURSE core_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h")

add_library(Core STATIC ${core_sources})
mupfel_apply_default_settings(Core)
mupfel_apply_strict_warnings(Core)

target_compile_features(Core PUBLIC cxx_std_23)

target_include_directories(Core
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/Include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Include/Core"
            "${CMAKE_CURRENT_SOURCE_DIR}/Include/ECS"
            "${CMAKE_CURRENT_SOURCE_DIR}/Include/ECS/Components"
            "${CMAKE_CURRENT_SOURCE_DIR}/Include/FS"
            "${CMAKE_CURRENT_SOURCE_DIR}/Include/Renderer"
            "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_link_libraries(Core
    PUBLIC  spdlog
            Mupfel::nlohmann
            Mupfel::glm
            imgui
    PRIVATE nvrhi_vk
            nvrhi
            vk-bootstrap
            box2d
            Mupfel::Vulkan
            Mupfel::GLFW
            Mupfel::stb
            $<$<PLATFORM_ID:Windows>:winmm>)

set_property(TARGET Core PROPERTY FOLDER "Engine")
