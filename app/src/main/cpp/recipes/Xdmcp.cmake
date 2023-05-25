check_function_exists("srand48" HAVE_SRAND48)
check_function_exists("lrand48" HAVE_LRAND48)
check_function_exists("arc4random_buf" HAVE_ARC4RANDOM_BUF)
check_function_exists("getentropy" HAVE_GETENTROPY)
check_include_file("sys/random.h" HAVE_SYS_RANDOM_H)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Xdmcp")
file(CONFIGURE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Xdmcp/config.h"
        CONTENT "
#pragma once
#define HASXDMAUTH 1
#cmakedefine01 HAVE_ARC4RANDOM_BUF
#cmakedefine01 HAVE_GETENTROPY
#cmakedefine01 HAVE_LRAND48
#cmakedefine01 HAVE_SRAND48
#cmakedefine01 HAVE_SYS_RANDOM_H
")

add_library(Xdmcp STATIC
        "libxdmcp/Array.c"
        "libxdmcp/Fill.c"
        "libxdmcp/Flush.c"
        "libxdmcp/Key.c"
        "libxdmcp/Read.c"
        "libxdmcp/Unwrap.c"
        "libxdmcp/Wrap.c"
        "libxdmcp/Wraphelp.c"
        "libxdmcp/Write.c")
target_compile_options(Xdmcp PRIVATE "-D_GNU_SOURCE=1" "-DHAVE_CONFIG_H" ${common_compile_options})
target_include_directories(Xdmcp PUBLIC "libxdmcp/include" "${CMAKE_CURRENT_BINARY_DIR}/Xdmcp")
target_link_libraries(Xdmcp PRIVATE xorgproto)