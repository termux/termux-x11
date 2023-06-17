check_include_file("sys/uio.h" HAVE_SYS_UIO_H)
check_include_file("dlfcn.h" HAVE_DLFCN_H)
check_include_file("sys/eventfd.h" HAVE_EVENTFD_H)
check_include_file("sys/select.h" HAVE_SYS_SELECT_H)

set (HAVE_PTHREAD 1)
set (HAVE_EPOXY_EGL_H 1)

check_source_compiles(C "
    #define _GNU_SOURCE 1
    #include <pthread.h>
    int main() { pthread_setname_np(pthread_self(), 0, NULL); }
" HAVE_PTHREAD_SETAFFINITY)
foreach(func "bswap32" "bswap64" "clz" "clzll" "expect" "ffs" "ffsll"
             "popcount" "popcountll" "types_compatible_p" "unreachable")
    string(TOUPPER "HAVE___BUILTIN_${func}" CHECK_NAME)
    check_function_exists("${func}" ${CHECK_NAME})
endforeach ()

if (ANDROID_PLATFORM)
    # Android defines it as a static inline, for some reason it is not detected
    set(HAVE___BUILTIN_FFSLL 1)
endif()

set(STUBMAIN "int main(int, char**){return 0;};")
check_source_compiles(C "__attribute__((const)) ${STUBMAIN}" HAVE_FUNC_ATTRIBUTE_CONST)
check_source_compiles(C "__attribute__((flatten)) ${STUBMAIN}" HAVE_FUNC_ATTRIBUTE_FLATTEN)
check_source_compiles(C "${STUBMAIN} __attribute__((format(printf, 1, 2))) int foo(const char * p, ...);" HAVE_FUNC_ATTRIBUTE_FORMAT)
check_source_compiles(C "__attribute__((malloc)) ${STUBMAIN}" HAVE_FUNC_ATTRIBUTE_MALLOC)
check_source_compiles(C "__attribute__((noreturn)) ${STUBMAIN}" HAVE_FUNC_ATTRIBUTE_NORETURN)
check_source_compiles(C "${STUBMAIN} struct __attribute__((packed)) foo { int bar; };" HAVE_FUNC_ATTRIBUTE_PACKED)
check_source_compiles(C "__attribute__((pure)) ${STUBMAIN}" HAVE_FUNC_ATTRIBUTE_PURE)
check_source_compiles(C "${STUBMAIN} __attribute__((returns_nonnull)) void* foo() {return (void*) 0; }" HAVE_FUNC_ATTRIBUTE_RETURNS_NONNULL)
check_source_compiles(C "__attribute__((unused)) ${STUBMAIN}" HAVE_FUNC_ATTRIBUTE_UNUSED)
check_source_compiles(C "__attribute__((warn_unused_result)) ${STUBMAIN}" HAVE_FUNC_ATTRIBUTE_WARN_UNUSED_RESULT)
check_source_compiles(C "__attribute__((weak)) ${STUBMAIN}" HAVE_FUNC_ATTRIBUTE_WEAK)
check_source_compiles(C "${STUBMAIN} __attribute__((alias(\"main\"))) int main1(int, char**);" HAVE_FUNC_ATTRIBUTE_ALIAS)
check_source_compiles(C "__attribute__((visibility(\"hidden\"))) ${STUBMAIN}" HAVE_FUNC_ATTRIBUTE_VISIBILITY)

check_function_exists("strtok_r" HAVE_STRTOK_R)
check_function_exists("timespec_get" HAVE_TIMESPEC_GET)

TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    set(UTIL_ARCH_LITTLE_ENDIAN 0)
    set(UTIL_ARCH_BIG_ENDIAN 1)
else ()
    set(UTIL_ARCH_LITTLE_ENDIAN 1)
    set(UTIL_ARCH_BIG_ENDIAN 0)
endif ()

check_source_compiles(C "#ifndef __i386__\n#error\n#endif\n${MESA_STUBMAIN}" PIPE_ARCH_X86)
check_source_compiles(C "#ifndef __x86_64__\n#error\n#endif\n${MESA_STUBMAIN}" PIPE_ARCH_X86_64)
check_source_compiles(C "#ifndef __arm__\n#error\n#endif\n${MESA_STUBMAIN}" PIPE_ARCH_ARM)
check_source_compiles(C "#ifndef __aarch64__\n#error\n#endif\n${MESA_STUBMAIN}" PIPE_ARCH_AARCH64)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/virglrenderer")
file(CONFIGURE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/virglrenderer/config.h"
        CONTENT "\
#pragma once
#cmakedefine VERSION
#define _GNU_SOURCE 1
#define VIRGL_RENDERER_UNSTABLE_APIS 1
#cmakedefine HAVE___BUILTIN_BSWAP32
#cmakedefine HAVE___BUILTIN_BSWAP64
#cmakedefine HAVE___BUILTIN_CLZ
#cmakedefine HAVE___BUILTIN_CLZLL
#cmakedefine HAVE___BUILTIN_EXPECT
#cmakedefine HAVE___BUILTIN_FFS
#cmakedefine HAVE___BUILTIN_FFSLL
#cmakedefine HAVE___BUILTIN_POPCOUNT
#cmakedefine HAVE___BUILTIN_POPCOUNTLL
#cmakedefine HAVE___BUILTIN_TYPES_COMPATIBLE_P
#cmakedefine HAVE___BUILTIN_UNREACHABLE
#cmakedefine HAVE_FUNC_ATTRIBUTE_CONST
#cmakedefine HAVE_FUNC_ATTRIBUTE_FLATTEN
#cmakedefine HAVE_FUNC_ATTRIBUTE_FORMAT
#cmakedefine HAVE_FUNC_ATTRIBUTE_MALLOC
#cmakedefine HAVE_FUNC_ATTRIBUTE_NORETURN
#cmakedefine HAVE_FUNC_ATTRIBUTE_PACKED
#cmakedefine HAVE_FUNC_ATTRIBUTE_PURE
#cmakedefine HAVE_FUNC_ATTRIBUTE_RETURNS_NONNULL
#cmakedefine HAVE_FUNC_ATTRIBUTE_UNUSED
#cmakedefine HAVE_FUNC_ATTRIBUTE_WARN_UNUSED_RESULT
#cmakedefine HAVE_FUNC_ATTRIBUTE_WEAK
#cmakedefine HAVE_MEMFD_CREATE
#cmakedefine HAVE_STRTOK_R
#cmakedefine HAVE_TIMESPEC_GET
#cmakedefine HAVE_SYS_UIO_H
#cmakedefine HAVE_PTHREAD
#cmakedefine HAVE_PTHREAD_SETAFFINITY
#cmakedefine01 HAVE_EPOXY_EGL_H
#cmakedefine HAVE_EPOXY_GLX_H
#cmakedefine CHECK_GL_ERRORS
#cmakedefine ENABLE_MINIGBM_ALLOCATION
#cmakedefine ENABLE_DRM
#cmakedefine ENABLE_DRM_MSM
#cmakedefine ENABLE_RENDER_SERVER
#cmakedefine ENABLE_RENDER_SERVER_WORKER_PROCESS
#cmakedefine ENABLE_RENDER_SERVER_WORKER_THREAD
#cmakedefine ENABLE_RENDER_SERVER_WORKER_MINIJAIL
#cmakedefine RENDER_SERVER_EXEC_PATH
#cmakedefine HAVE_EVENTFD_H
#cmakedefine HAVE_DLFCN_H
#cmakedefine ENABLE_VIDEO
#cmakedefine ENABLE_TRACING
#cmakedefine01 UTIL_ARCH_LITTLE_ENDIAN
#cmakedefine01 UTIL_ARCH_BIG_ENDIAN
#cmakedefine PIPE_ARCH_X86
#cmakedefine PIPE_ARCH_X86_64
#cmakedefine PIPE_ARCH_PPC
#cmakedefine PIPE_ARCH_PPC_64
#cmakedefine PIPE_ARCH_S390
#cmakedefine PIPE_ARCH_ARM
#cmakedefine PIPE_ARCH_AARCH64
#define EGL_WITHOUT_GBM

#cmakedefine ENABLE_TRACING \"@VIRGL_WITH_TRACING@\"

#undef ANDROID

#include <assert.h>
")

file(CONFIGURE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/virglrenderer/gl4es-decompress.c"
        CONTENT "#include \"${CMAKE_CURRENT_SOURCE_DIR}/virglrenderer/src/gl4es-decompress.c\"")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/virglrenderer/u_format_table.c"
        COMMAND Python3::Interpreter "-B" "${CMAKE_CURRENT_SOURCE_DIR}/virglrenderer/src/gallium/auxiliary/util/u_format_table.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/virglrenderer/src/gallium/auxiliary/util/u_format.csv" ">"
            "${CMAKE_CURRENT_BINARY_DIR}/virglrenderer/u_format_table.c"
        COMMENT "Generating source code (u_format_table.c)"
        VERBATIM)

add_executable(virgl_test_server
        "virglrenderer/src/virglrenderer.c"

        "virglrenderer/src/iov.c"
        "virglrenderer/src/virgl_context.c"
        "virglrenderer/src/virgl_resource.c"
        "virglrenderer/src/virgl_util.c"

        "virglrenderer/src/vrend_winsys_egl.c"

        "${CMAKE_CURRENT_BINARY_DIR}/virglrenderer/gl4es-decompress.c"
        "virglrenderer/src/vrend_blitter.c"
        "virglrenderer/src/vrend_debug.c"
        "virglrenderer/src/vrend_decode.c"
        "virglrenderer/src/vrend_formats.c"
        "virglrenderer/src/vrend_object.c"
        "virglrenderer/src/vrend_renderer.c"
        "virglrenderer/src/vrend_shader.c"
        "virglrenderer/src/vrend_tweaks.c"
        "virglrenderer/src/vrend_winsys.c"

        "virglrenderer/src/mesa/util/anon_file.c"
        "virglrenderer/src/mesa/util/bitscan.c"
        "virglrenderer/src/mesa/util/hash_table.c"
        "virglrenderer/src/mesa/util/os_file.c"
        "virglrenderer/src/mesa/util/os_misc.c"
        "virglrenderer/src/mesa/util/ralloc.c"
        "virglrenderer/src/mesa/util/u_cpu_detect.c"
        "virglrenderer/src/mesa/util/u_debug.c"
        "virglrenderer/src/mesa/util/u_math.c"

        "${CMAKE_CURRENT_BINARY_DIR}/virglrenderer/u_format_table.c"
        "virglrenderer/src/gallium/auxiliary/util/u_format.c"
        "virglrenderer/src/gallium/auxiliary/util/u_texture.c"
        "virglrenderer/src/gallium/auxiliary/util/u_hash_table.c"
        "virglrenderer/src/gallium/auxiliary/util/u_debug_describe.c"
        "virglrenderer/src/gallium/auxiliary/cso_cache/cso_cache.c"
        "virglrenderer/src/gallium/auxiliary/cso_cache/cso_hash.c"
        "virglrenderer/src/gallium/auxiliary/tgsi/tgsi_dump.c"
        "virglrenderer/src/gallium/auxiliary/tgsi/tgsi_build.c"
        "virglrenderer/src/gallium/auxiliary/tgsi/tgsi_scan.c"
        "virglrenderer/src/gallium/auxiliary/tgsi/tgsi_info.c"
        "virglrenderer/src/gallium/auxiliary/tgsi/tgsi_parse.c"
        "virglrenderer/src/gallium/auxiliary/tgsi/tgsi_text.c"
        "virglrenderer/src/gallium/auxiliary/tgsi/tgsi_strings.c"
        "virglrenderer/src/gallium/auxiliary/tgsi/tgsi_sanity.c"
        "virglrenderer/src/gallium/auxiliary/tgsi/tgsi_iterate.c"
        "virglrenderer/src/gallium/auxiliary/tgsi/tgsi_util.c"

        "virglrenderer/vtest/util.c"
        "virglrenderer/vtest/vtest_shm.c"
        "virglrenderer/vtest/vtest_server.c"
        "virglrenderer/vtest/vtest_renderer.c")
target_include_directories(virgl_test_server PRIVATE
        "libepoxy/include"
        "drm/include/drm"
        "virglrenderer/src"
        "virglrenderer/src/gallium/auxiliary"
        "virglrenderer/src/gallium/auxiliary/util"
        "virglrenderer/src/gallium/include"
        "virglrenderer/src/drm"
        "virglrenderer/src/mesa"
        "virglrenderer/src/mesa/compat"
        "virglrenderer/src/mesa/pipe"
        "virglrenderer/src/mesa/util"
        "virglrenderer/src/venus"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/virglrenderer")
target_compile_options(virgl_test_server PRIVATE "-fPIE" "-DHAVE_CONFIG_H" "-imacros" "${CMAKE_CURRENT_BINARY_DIR}/virglrenderer/config.h")
target_link_libraries(virgl_test_server PRIVATE epoxy m ${VIRGL_VENUS_VULKAN_LIBRARY})
target_link_options(virgl_test_server PRIVATE "-s")

file(GLOB virglrenderer_patches "patches/virglrenderer/*.patc*")
foreach(patch ${virglrenderer_patches})
    target_apply_patch(virgl_test_server "${CMAKE_CURRENT_SOURCE_DIR}/virglrenderer" "${patch}")
endforeach()
