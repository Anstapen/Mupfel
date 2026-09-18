# cmake/GLFW.cmake
#
# GLFW is the one dependency that is not vendored the same way on both platforms, so it cannot be
# consumed unconditionally: Windows uses the prebuilt binary fetched as the `glfw` dep, Linux links the
# system libglfw.so from libglfw3-dev -- which is -lglfw, not -lglfw3.
#
# The Premake build repeated that split in Core and again in Tests. Here it is one interface target,
# Mupfel::GLFW, that both link.

add_library(Mupfel_GLFW INTERFACE)
add_library(Mupfel::GLFW ALIAS Mupfel_GLFW)

if(WIN32)
    target_include_directories(Mupfel_GLFW SYSTEM INTERFACE "${MUPFEL_DEP_glfw_DIR}/include")
    target_link_libraries(Mupfel_GLFW INTERFACE "${MUPFEL_DEP_glfw_DIR}/lib-vc2022/glfw3.lib")

    # glfw3.lib's Win32 backend imports from gdi32 (CreateDIBSection, SwapBuffers, ...). MSBuild linked
    # it through its default CoreLibraryDependencies, so the Premake build only needed this under the
    # ninja action; naming it here makes the requirement independent of the generator.
    target_link_libraries(Mupfel_GLFW INTERFACE gdi32)
else()
    # Headers come from the default search path (libglfw3-dev), so there is no include directory to
    # add. `dl` is vk-bootstrap's: it does not link the Vulkan loader, it dlopen()s libvulkan.so.1 and
    # resolves entry points through dlsym.
    target_link_libraries(Mupfel_GLFW INTERFACE glfw dl)
endif()
