check_include_file("err.h" HAVE_ERR_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_function_exists("readlink" HAVE_READLINK)
check_function_exists("reallocarray" HAVE_REALLOCARRAY)
check_function_exists("realpath" HAVE_REALPATH)
check_function_exists("strlcpy" HAVE_STRLCPY)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Xfont2")
file(CONFIGURE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Xfont2/config.h
        CONTENT "
#pragma once
#cmakedefine HAVE_ERR_H
#cmakedefine01 HAVE_STDINT_H
#cmakedefine HAVE_READLINK
#cmakedefine HAVE_REALLOCARRAY
#cmakedefine HAVE_REALPATH
#cmakedefine HAVE_STRLCPY

#define XFONT_BDFFORMAT 1
#define XFONT_BITMAP 1
#define XFONT_FC 1
#define XFONT_FREETYPE 1
#define XFONT_PCFFORMAT 1
#undef XFONT_SNFFORMAT
#undef X_BZIP2_FONT_COMPRESSION
#define X_GZIP_FONT_COMPRESSION 1

#if defined(ANDROID) && defined(HAVE_REALLOCARRAY)
#include <malloc.h>
#endif
")

if(NOT HAVE_STRLCPY)
    set(XFONT_STRLCPY "libxfont/src/util/strlcat.c")
else()
    set(XFONT_STRLCPY)
endif ()

if(NOT HAVE_REALLOCARRAY)
    set(XFONT_REALLOCARRAY "libxfont/src/util/reallocarray.c")
else()
    set(XFONT_REALLOCARRAY)
endif ()

add_library(Xfont2 STATIC
        "libxfont/src/stubs/atom.c"
        "libxfont/src/stubs/libxfontstubs.c"
        "libxfont/src/util/fontaccel.c"
        "libxfont/src/util/fontnames.c"
        "libxfont/src/util/fontutil.c"
        "libxfont/src/util/fontxlfd.c"
        "libxfont/src/util/format.c"
        "libxfont/src/util/miscutil.c"
        "libxfont/src/util/patcache.c"
        "libxfont/src/util/private.c"
        "libxfont/src/util/utilbitmap.c"
        ${XFONT_STRLCPY}
        ${XFONT_REALLOCARRAY}

        "libxfont/src/fontfile/bitsource.c"
        "libxfont/src/fontfile/bufio.c"
        "libxfont/src/fontfile/decompress.c"
        "libxfont/src/fontfile/defaults.c"
        "libxfont/src/fontfile/dirfile.c"
        "libxfont/src/fontfile/fileio.c"
        "libxfont/src/fontfile/filewr.c"
        "libxfont/src/fontfile/fontdir.c"
        "libxfont/src/fontfile/fontencc.c"
        "libxfont/src/fontfile/fontfile.c"
        "libxfont/src/fontfile/fontscale.c"
        "libxfont/src/fontfile/gunzip.c"
        "libxfont/src/fontfile/register.c"
        "libxfont/src/fontfile/renderers.c"
        "libxfont/src/fontfile/catalogue.c"

        "libxfont/src/FreeType/ftenc.c"
        "libxfont/src/FreeType/ftfuncs.c"
        "libxfont/src/FreeType/fttools.c"
        "libxfont/src/FreeType/xttcap.c"

        "libxfont/src/bitmap/bitmap.c"
        "libxfont/src/bitmap/bitmapfunc.c"
        "libxfont/src/bitmap/bitmaputil.c"
        "libxfont/src/bitmap/bitscale.c"
        "libxfont/src/bitmap/fontink.c"

        "libxfont/src/bitmap/bdfread.c"
        "libxfont/src/bitmap/bdfutils.c"

        "libxfont/src/bitmap/pcfread.c"

        "libxfont/src/bitmap/pcfwrite.c"

        "libxfont/src/builtins/dir.c"
        "libxfont/src/builtins/file.c"
        "libxfont/src/builtins/fonts.c"
        "libxfont/src/builtins/fpe.c"
        "libxfont/src/builtins/render.c"

        "libxfont/src/fc/fsconvert.c"
        "libxfont/src/fc/fserve.c"
        "libxfont/src/fc/fsio.c"
        "libxfont/src/fc/fstrans.c")
target_compile_options(Xfont2 PRIVATE
        ${common_compile_options}
        "-fvisibility=hidden"
        "-DHAVE_CONFIG_H"
        "-D_GNU_SOURCE=1"
        "-D_DEFAULT_SOURCE=1"
        "-D_BSD_SOURCE=1"
        "-DHAS_FCHOWN"
        "-DHAS_STICKY_DIR_BIT"
        "-D_XOPEN_SOURCE"
        "-DNOFILES_MAX=512")
target_include_directories(Xfont2 PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/Xfont2"
        "libxfont"
        "libxfont/include"
        "libfontenc/include")
target_link_libraries(Xfont2 PUBLIC freetype xorgproto Xtrans)