add_library(Xxf86vm STATIC "libxxf86vm/src/XF86VMode.c")
target_include_directories(Xxf86vm PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "xorgproto/include"
        "libx11/include"
        "libxext/include"
        "libxxf86vm/include")
