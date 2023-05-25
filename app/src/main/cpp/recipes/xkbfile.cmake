add_library(xkbfile STATIC
        libxkbfile/src/cout.c
        libxkbfile/src/maprules.c
        libxkbfile/src/srvmisc.c
        libxkbfile/src/xkbatom.c
        libxkbfile/src/xkbbells.c
        libxkbfile/src/xkbconfig.c
        libxkbfile/src/xkbdraw.c
        libxkbfile/src/xkberrs.c
        libxkbfile/src/xkbmisc.c
        libxkbfile/src/xkbout.c
        libxkbfile/src/xkbtext.c
        libxkbfile/src/xkmout.c
        libxkbfile/src/xkmread.c)
target_include_directories(xkbfile PUBLIC "libxkbfile/include" PRIVATE "libxkbfile/include/X11/extensions")
target_link_libraries(xkbfile xorgproto X11)
target_compile_options(xkbfile PRIVATE "-fPIE" "-fPIC")