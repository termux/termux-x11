# Normally xkbcomp is build as executable, but in our case it is better to embed it.

check_function_exists("strcasecmp" HAVE_STRCASECMP)
check_function_exists("strdup" HAVE_STRDUP)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/xkbcomp-config")
file(CONFIGURE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xkbcomp-config/config.h
        CONTENT "
#pragma once
#cmakedefine HAVE_STRCASECMP
#cmakedefine HAVE_STRDUP
#define PACKAGE_VERSION \"1.4.6\"

extern char* DFLT_XKB_CONFIG_ROOT;
")

file(CONFIGURE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xkbcomp-config/xkbcomp-defs.c"
        CONTENT "
#include <unistd.h>

int asprintf(char **restrict strp, const char *restrict fmt, ...);
char *getenv(const char *name);

char* DFLT_XKB_CONFIG_ROOT = NULL;
extern char* __progname;

__attribute__((constructor))
static void init_pathes(void) {
    if (getenv(\"XKB_CONFIG_ROOT\"))
        DFLT_XKB_CONFIG_ROOT = getenv(\"XKB_CONFIG_ROOT\");
    else
        DFLT_XKB_CONFIG_ROOT = (char*) \"/usr/share/X11/xkb\";
}
")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/X11")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/patches/ks_tables.h" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/X11")

add_library(X11 STATIC
        "libx11/src/KeyBind.c"
        "libx11/src/KeysymStr.c"
        "libx11/src/Quarks.c"
        "libx11/src/StrKeysym.c"
        "libx11/src/Xrm.c"

        "libx11/src/xkb/XKBGeom.c"
        "libx11/src/xkb/XKBMisc.c"
        "libx11/src/xkb/XKBMAlloc.c"
        "libx11/src/xkb/XKBGAlloc.c"
        "libx11/src/xkb/XKBAlloc.c")
target_link_libraries(X11 PUBLIC xcb xcbproto)
target_link_options(X11 PRIVATE "-fPIC")
target_include_directories(X11
        PUBLIC
        "libx11/include"
        PRIVATE
        "libx11/include/X11"
        "libx11/src"
        "libx11/src/xlibi18n"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/X11")
target_compile_options(X11 PRIVATE ${common_compile_options} "-fvisibility=hidden" "-DHAVE_SYS_IOCTL_H" "-fPIE" "-fPIC")

add_library(xkbfile STATIC
        libxkbfile/src/xkbatom.c
        libxkbfile/src/xkberrs.c
        libxkbfile/src/xkbmisc.c
        libxkbfile/src/xkbout.c
        libxkbfile/src/xkbtext.c
        libxkbfile/src/xkmout.c)
target_include_directories(xkbfile PUBLIC "libxkbfile/include" PRIVATE "libxkbfile/include/X11/extensions")
target_link_libraries(xkbfile xorgproto X11)
target_compile_options(xkbfile PRIVATE "-fPIE" "-fPIC")

BISON_TARGET(xkbcomp_parser xkbcomp/xkbparse.y ${CMAKE_CURRENT_BINARY_DIR}/xkbparse.c)
add_library(xkbcomp STATIC
        xkbcomp/action.c
        xkbcomp/alias.c
        xkbcomp/compat.c
        xkbcomp/expr.c
        xkbcomp/geometry.c
        xkbcomp/indicators.c
        xkbcomp/keycodes.c
        xkbcomp/keymap.c
        xkbcomp/keytypes.c
        xkbcomp/listing.c
        xkbcomp/misc.c
        xkbcomp/parseutils.c
        xkbcomp/symbols.c
        xkbcomp/utils.c
        xkbcomp/vmod.c
        xkbcomp/xkbcomp.c
        xkbcomp/xkbparse.y
        xkbcomp/xkbpath.c
        xkbcomp/xkbscan.c
        ${CMAKE_CURRENT_BINARY_DIR}/xkbparse.c
        "${CMAKE_CURRENT_BINARY_DIR}/xkbcomp-config/xkbcomp-defs.c")
target_compile_options(xkbcomp PRIVATE "-DHAVE_CONFIG_H" ${common_compile_options} "-fPIC")
target_link_options(xkbcomp PRIVATE "-fPIC")
target_include_directories(xkbcomp PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/xkbcomp-config" "xkbcomp")
target_link_libraries(xkbcomp PRIVATE X11 xkbfile)
target_apply_patch(xkbcomp "${CMAKE_CURRENT_SOURCE_DIR}/xkbcomp" "${CMAKE_CURRENT_SOURCE_DIR}/patches/xkbcomp.patch")
target_apply_patch(xkbfile "${CMAKE_CURRENT_SOURCE_DIR}/libxkbfile" "${CMAKE_CURRENT_SOURCE_DIR}/patches/xkbfile.patch")
target_apply_patch(X11 "${CMAKE_CURRENT_SOURCE_DIR}/libx11" "${CMAKE_CURRENT_SOURCE_DIR}/patches/x11.patch")
