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
