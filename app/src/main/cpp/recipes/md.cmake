file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/libmd/config.h" CONTENT "")
add_library(md STATIC
        libmd/src/md2.c
        libmd/src/md4.c
        libmd/src/md5.c
        libmd/src/rmd160.c
        libmd/src/sha1.c
        libmd/src/sha2.c)
target_include_directories(md PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/libmd" PUBLIC "libmd/include")