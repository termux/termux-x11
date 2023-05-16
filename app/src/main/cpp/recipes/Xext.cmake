add_library(Xext STATIC
        "libxext/src/DPMS.c"
        "libxext/src/MITMisc.c"
        "libxext/src/XAppgroup.c"
        "libxext/src/XEVI.c"
        "libxext/src/XLbx.c"
        "libxext/src/XMultibuf.c"
        "libxext/src/XSecurity.c"
        "libxext/src/XShape.c"
        "libxext/src/XShm.c"
        "libxext/src/XSync.c"
        "libxext/src/XTestExt1.c"
        "libxext/src/Xcup.c"
        "libxext/src/Xdbe.c"
        "libxext/src/Xge.c"
        "libxext/src/extutil.c"
        "libxext/src/globals.c")
target_include_directories(Xext PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "libxext/include"
        "xorgproto/include"
        "libx11/include"
        "libxau/include")
target_compile_options(Xext PRIVATE "-fPIC")
