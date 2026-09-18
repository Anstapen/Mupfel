# App/CMakeLists.txt
#
# Builds the "App" executable: game/editor-specific layers (see App/Source) plus the shaders the engine
# dispatches (see App/Shaders). Entry point is #include "Mupfel.h".
#
# Note how short the dependency list is. App names Core and nothing else: the published header root,
# spdlog and nlohmann json arrive as Core's PUBLIC usage requirements, and every other vendored library
# arrives on the link line without its include directories, because Core declares it PRIVATE. The
# Premake build had to spell this out as a hand-maintained includedirs list that was correct only as
# long as nobody forgot to update it -- and, under the ninja action, as a second copy of Core's entire
# link list (ApplyCoreLinkDependencies), because Premake's ar rule dropped Core's own dependencies.
#
# The result is the same as before and is still worth stating: NVRHI, vk-bootstrap, the Vulkan SDK,
# GLFW, ImGui, Box2D and glm are not reachable from application code.

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

# Carried as sources so they show up in the IDE alongside the code that dispatches them, and marked
# header-only so no generator tries to compile them. The Premake build globbed "Shaders/**.glsl", which
# has matched nothing since the shaders became Slang -- they are built out-of-band by
# App/Shaders/compile.bat, which this does not change.
file(GLOB app_shaders CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.slang")
if(app_shaders)
    target_sources(App PRIVATE ${app_shaders})
    set_source_files_properties(${app_shaders} PROPERTIES HEADER_FILE_ONLY ON)
    source_group("Shaders" FILES ${app_shaders})
endif()

# Run from App/, matching where the Premake solution started it, so relative asset and log paths
# resolve the same way under F5 in the IDE.
set_property(TARGET App PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
