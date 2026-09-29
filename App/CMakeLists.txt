# App/CMakeLists.txt
#
# Builds the "App" executable: game/editor-specific layers (see App/Source)

file(GLOB_RECURSE app_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h")

add_executable(App ${app_sources})
mupfel_apply_default_settings(App)

target_compile_features(App PRIVATE cxx_std_23)
target_include_directories(App PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_link_libraries(App PRIVATE Core)

if(WIN32)
    target_compile_definitions(App PRIVATE WINDOWS)
endif()

# Run from App/, so relative asset and log paths resolve the same way in the IDE.
set_property(TARGET App PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
