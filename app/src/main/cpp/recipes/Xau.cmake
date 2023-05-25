check_function_exists("explicit_bzero" HAVE_EXPLICIT_BZERO)
check_function_exists("pathconf" HAVE_PATHCONF)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Xau")
file(CONFIGURE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Xau/config.h
        CONTENT "
#pragma once
#cmakedefine HAVE_EXPLICIT_BZERO
#cmakedefine HAVE_PATHCONF
#define XTHREADS 1
#define XUSE_MTSAFE_API 1
")

add_library(Xau STATIC
        "libxau/AuDispose.c"
        "libxau/AuFileName.c"
        "libxau/AuGetAddr.c"
        "libxau/AuGetBest.c"
        "libxau/AuLock.c"
        "libxau/AuRead.c"
        "libxau/AuUnlock.c"
        "libxau/AuWrite.c")
target_include_directories(Xau PUBLIC "libxau/include")
target_compile_options(Xdmcp PRIVATE "-D_GNU_SOURCE=1" "-DHAVE_CONFIG_H" ${common_compile_options} "-fPIC")
target_link_libraries(Xau xcbproto xorgproto)
