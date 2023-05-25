add_library(Xfixes STATIC
        "libxfixes/src/Cursor.c"
        "libxfixes/src/Disconnect.c"
        "libxfixes/src/Region.c"
        "libxfixes/src/SaveSet.c"
        "libxfixes/src/Selection.c"
        "libxfixes/src/Xfixes.c")
target_include_directories(Xfixes PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "xorgproto/include"
        "libx11/include"
        "libxfixes/include/X11/extensions")
