add_library(Mupfel_nlohmann INTERFACE)
add_library(Mupfel::nlohmann ALIAS Mupfel_nlohmann)
target_include_directories(Mupfel_nlohmann SYSTEM INTERFACE "${MUPFEL_DEP_nlohmann_DIR}")

add_library(Mupfel_glm INTERFACE)
add_library(Mupfel::glm ALIAS Mupfel_glm)
target_include_directories(Mupfel_glm SYSTEM INTERFACE "${MUPFEL_DEP_glm_DIR}")
target_compile_definitions(Mupfel_glm INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE)

add_library(Mupfel_stb INTERFACE)
add_library(Mupfel::stb ALIAS Mupfel_stb)
target_include_directories(Mupfel_stb SYSTEM INTERFACE "${MUPFEL_DEP_stb_DIR}")

file(GLOB_RECURSE spdlog_sources "${MUPFEL_DEP_spdlog_DIR}/src/*.cpp")
add_library(spdlog STATIC ${spdlog_sources})
mupfel_apply_default_settings(spdlog)

target_compile_features(spdlog PRIVATE cxx_std_23)
target_include_directories(spdlog SYSTEM PUBLIC "${MUPFEL_DEP_spdlog_DIR}/include")

target_compile_definitions(spdlog PUBLIC SPDLOG_COMPILED_LIB)

set_property(TARGET spdlog PROPERTY FOLDER "Vendor")

file(GLOB imgui_sources "${MUPFEL_DEP_imgui_DIR}/*.cpp")
add_library(imgui STATIC
    ${imgui_sources}
    "${MUPFEL_DEP_imgui_DIR}/backends/imgui_impl_glfw.cpp")
mupfel_apply_default_settings(imgui)

target_compile_features(imgui PRIVATE cxx_std_23)
target_include_directories(imgui SYSTEM PUBLIC
    "${MUPFEL_DEP_imgui_DIR}"
    "${MUPFEL_DEP_imgui_DIR}/backends")

target_link_libraries(imgui PRIVATE Mupfel::GLFW)

set_property(TARGET imgui PROPERTY FOLDER "Vendor")

file(GLOB nvrhi_validation_sources "${MUPFEL_DEP_nvrhi_DIR}/src/validation/*.cpp")
add_library(nvrhi STATIC
    "${MUPFEL_DEP_nvrhi_DIR}/src/common/format-info.cpp"
    "${MUPFEL_DEP_nvrhi_DIR}/src/common/misc.cpp"
    "${MUPFEL_DEP_nvrhi_DIR}/src/common/state-tracking.cpp"
    "${MUPFEL_DEP_nvrhi_DIR}/src/common/utils.cpp"
    "${MUPFEL_DEP_nvrhi_DIR}/src/common/aftermath.cpp"
    ${nvrhi_validation_sources})
mupfel_apply_default_settings(nvrhi)

target_compile_features(nvrhi PRIVATE cxx_std_17)
target_include_directories(nvrhi SYSTEM PUBLIC "${MUPFEL_DEP_nvrhi_DIR}/include")

target_compile_definitions(nvrhi PUBLIC NVRHI_WITH_AFTERMATH=0)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    target_sources(nvrhi PRIVATE "${MUPFEL_DEP_nvrhi_DIR}/tools/nvrhi.natvis")
endif()

set_property(TARGET nvrhi PROPERTY FOLDER "Vendor")

file(GLOB nvrhi_vk_sources "${MUPFEL_DEP_nvrhi_DIR}/src/vulkan/*.cpp")
add_library(nvrhi_vk STATIC ${nvrhi_vk_sources})
mupfel_apply_default_settings(nvrhi_vk)

target_compile_features(nvrhi_vk PRIVATE cxx_std_17)
target_link_libraries(nvrhi_vk PUBLIC nvrhi Mupfel::VulkanHeaders)

target_compile_definitions(nvrhi_vk PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

if(WIN32)
    target_compile_definitions(nvrhi_vk PUBLIC VK_USE_PLATFORM_WIN32_KHR NOMINMAX)
endif()

set_property(TARGET nvrhi_vk PROPERTY FOLDER "Vendor")

add_library(vk-bootstrap STATIC "${MUPFEL_DEP_vk_bootstrap_DIR}/src/VkBootstrap.cpp")
mupfel_apply_default_settings(vk-bootstrap)

target_compile_features(vk-bootstrap PRIVATE cxx_std_17)
target_include_directories(vk-bootstrap SYSTEM PUBLIC "${MUPFEL_DEP_vk_bootstrap_DIR}/src")
target_link_libraries(vk-bootstrap PUBLIC Mupfel::VulkanHeaders)

if(WIN32)
    target_compile_definitions(vk-bootstrap PUBLIC VK_USE_PLATFORM_WIN32_KHR NOMINMAX)
endif()

set_property(TARGET vk-bootstrap PROPERTY FOLDER "Vendor")

file(GLOB_RECURSE box2d_sources "${MUPFEL_DEP_box2d_DIR}/src/*.c")
add_library(box2d STATIC ${box2d_sources})
mupfel_apply_default_settings(box2d)

target_include_directories(box2d SYSTEM PUBLIC "${MUPFEL_DEP_box2d_DIR}/include")
target_include_directories(box2d PRIVATE "${MUPFEL_DEP_box2d_DIR}/src")

if(WIN32)
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        target_sources(box2d PRIVATE "${MUPFEL_DEP_box2d_DIR}/src/box2d.natvis")
    endif()

    set_target_properties(box2d PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
else()
    set_target_properties(box2d PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED ON C_EXTENSIONS ON)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
elseif(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(box2d PRIVATE /clang:-ffp-contract=off)
else()
    target_compile_options(box2d PRIVATE -ffp-contract=off)
endif()

if(MSVC)
    target_compile_options(box2d PRIVATE $<$<CONFIG:Debug>:/O2>)

    set_property(TARGET box2d PROPERTY MSVC_RUNTIME_CHECKS "")
else()
    target_compile_options(box2d PRIVATE $<$<CONFIG:Debug>:-O3>)
endif()

set_property(TARGET box2d PROPERTY FOLDER "Vendor")

if(MUPFEL_MODULE_TESTS)
    add_library(catch2 STATIC "${MUPFEL_DEP_catch2_DIR}/catch_amalgamated.cpp")
    mupfel_apply_default_settings(catch2)

    target_compile_features(catch2 PRIVATE cxx_std_23)
    target_include_directories(catch2 SYSTEM PUBLIC "${MUPFEL_DEP_catch2_DIR}")
    target_compile_definitions(catch2 PRIVATE DO_NOT_USE_WMAIN)

    set_property(TARGET catch2 PROPERTY FOLDER "Vendor")
endif()
