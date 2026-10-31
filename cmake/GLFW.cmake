add_library(Mupfel_GLFW INTERFACE)
add_library(Mupfel::GLFW ALIAS Mupfel_GLFW)

if(WIN32)
    target_include_directories(Mupfel_GLFW SYSTEM INTERFACE "${MUPFEL_DEP_glfw_DIR}/include")
    target_link_libraries(Mupfel_GLFW INTERFACE "${MUPFEL_DEP_glfw_DIR}/lib-vc2022/glfw3.lib")

    target_link_libraries(Mupfel_GLFW INTERFACE gdi32)
else()
    target_link_libraries(Mupfel_GLFW INTERFACE glfw dl)
endif()
