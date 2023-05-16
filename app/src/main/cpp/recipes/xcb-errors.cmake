file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/xcb-errors")
add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xcb-errors/extensions.c"
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/libxcb-errors/src/extensions.py"
            "${CMAKE_CURRENT_BINARY_DIR}/xcb-errors/extensions.c" ${xcbprotos}
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libxcb-errors/src/extensions.py" ${xcbprotos}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating errors database from XML"
        VERBATIM
)
add_library(xcb-errors STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/libxcb-errors/src/xcb_errors.c"
        "${CMAKE_CURRENT_BINARY_DIR}/xcb-errors/extensions.c")
target_include_directories(xcb-errors
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src"
            "${CMAKE_CURRENT_BINARY_DIR}"
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/libxcb-errors/src")
target_link_libraries(xcb-errors xcbproto)
 
