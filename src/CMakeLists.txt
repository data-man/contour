if(("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang"))
    #(TODO: first get rid of these warnings) add_compile_options(-Werror)
endif()

add_subdirectory(crispy)
add_subdirectory(text_shaper)
add_subdirectory(terminal)
add_subdirectory(terminal_renderer)
add_subdirectory(terminal_renderer/opengl)
add_subdirectory(terminal_view)

if(CONTOUR_CLIENT)
    add_subdirectory(contour)
endif()
