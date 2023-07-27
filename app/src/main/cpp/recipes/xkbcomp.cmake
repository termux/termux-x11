# Normally xkbcomp is build as executable, but in our case it is better to embed it.

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/X11")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/patches/ks_tables.h" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

BISON_TARGET(xkbcomp_parser xkbcomp/xkbparse.y ${CMAKE_CURRENT_BINARY_DIR}/xkbparse.c)
add_library(xkbcomp STATIC
        "libx11/src/KeyBind.c"
        "libx11/src/KeysymStr.c"
        "libx11/src/Quarks.c"
        "libx11/src/StrKeysym.c"
        "libx11/src/Xrm.c"

        "libx11/src/xkb/XKBGeom.c"
        "libx11/src/xkb/XKBMisc.c"
        "libx11/src/xkb/XKBMAlloc.c"
        "libx11/src/xkb/XKBGAlloc.c"
        "libx11/src/xkb/XKBAlloc.c"

        "libxkbfile/src/xkbatom.c"
        "libxkbfile/src/xkberrs.c"
        "libxkbfile/src/xkbmisc.c"
        "libxkbfile/src/xkbout.c"
        "libxkbfile/src/xkbtext.c"
        "libxkbfile/src/xkmout.c"

        "xkbcomp/action.c"
        "xkbcomp/alias.c"
        "xkbcomp/compat.c"
        "xkbcomp/expr.c"
        "xkbcomp/geometry.c"
        "xkbcomp/indicators.c"
        "xkbcomp/keycodes.c"
        "xkbcomp/keymap.c"
        "xkbcomp/keytypes.c"
        "xkbcomp/listing.c"
        "xkbcomp/misc.c"
        "xkbcomp/parseutils.c"
        "xkbcomp/symbols.c"
        "xkbcomp/utils.c"
        "xkbcomp/vmod.c"
        "xkbcomp/xkbcomp.c"
        "xkbcomp/xkbparse.y"
        "xkbcomp/xkbpath.c"
        "xkbcomp/xkbscan.c"
        "${CMAKE_CURRENT_BINARY_DIR}/xkbparse.c")
target_include_directories(xkbcomp
        PUBLIC
        "libxkbfile/include"
        PRIVATE
        "xkbcomp"
        "libx11/include"
        "libx11/include/X11"
        "libx11/src"
        "libx11/src/xlibi18n"
        "libxkbfile/include/X11/extensions"
        "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(xkbcomp PRIVATE xorgproto)
target_link_options(xkbcomp PRIVATE "-fPIE" "-fPIC")
target_compile_options(xkbcomp PRIVATE ${common_compile_options} "-fvisibility=hidden" "-DHAVE_STRCASECMP" "-DHAVE_STRDUP" "-DDFLT_XKB_CONFIG_ROOT=\"/\"" "-DHAVE_SYS_IOCTL_H" "-fPIE" "-fPIC")
target_apply_patch(xkbcomp "${CMAKE_CURRENT_SOURCE_DIR}/xkbcomp" "${CMAKE_CURRENT_SOURCE_DIR}/patches/xkbcomp.patch")
target_apply_patch(xkbfile "${CMAKE_CURRENT_SOURCE_DIR}/libxkbfile" "${CMAKE_CURRENT_SOURCE_DIR}/patches/xkbfile.patch")
target_apply_patch(X11 "${CMAKE_CURRENT_SOURCE_DIR}/libx11" "${CMAKE_CURRENT_SOURCE_DIR}/patches/x11.patch")
