file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/X11")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/X11/XlibConf.h" CONTENT "\n#pragma once\n#define XTHREADS 1\n#define XUSE_MTSAFE_API 1")

add_library(xorgproto INTERFACE)
target_include_directories(xorgproto INTERFACE "xorgproto/include")
set(USE_FDS_BITS 1)
configure_file("xorgproto/include/X11/Xpoll.h.in" "${CMAKE_CURRENT_BINARY_DIR}/X11/Xpoll.h" @ONLY)
