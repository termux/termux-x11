set(test_drm_compile_options
        "-Wsign-compare"
        "-Werror=undef"
        "-Werror=implicit-function-declaration"
        "-Wpointer-arith"
        "-Wwrite-strings"
        "-Wstrict-prototypes"
        "-Wmissing-prototypes"
        "-Wmissing-declarations"
        "-Wnested-externs"
        "-Wpacked"
        "-Wswitch-enum"
        "-Wmissing-format-attribute"
        "-Wstrict-aliasing=2"
        "-Winit-self"
        "-Winline"
        "-Wshadow"
        "-Wdeclaration-after-statement"
        "-Wold-style-definition"

        "-Wno-unused-parameter"
        "-Wno-attributes"
        "-Wno-long-long"
        "-Wno-missing-field-initializers")

set(CMAKE_REQUIRED_QUIET_OLD ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET 1)
set(drm_compile_options)
set(I 0)
foreach (option ${test_drm_compile_options})
    check_c_compiler_flag(${option} DRM_COMPILER_TEST_${I})
    if (DRM_COMPILER_TEST_${I})
        set(drm_compile_options ${common_compile_options} ${option})
    endif()
    MATH(EXPR I "${I}+1")
endforeach ()
set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_OLD})

#HAVE_LIBDRM_ATOMIC_PRIMITIVES
#HAVE_LIB_ATOMIC_OPS
check_include_file("sys/sysctl.h" HAVE_SYS_SYSCTL_H)
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
check_include_file("alloca.h" HAVE_ALLOCA_H)

check_symbol_exists("major" "sys/sysmacros.h" HAS_MAJOR_IN_SYSMACROS)
check_symbol_exists("minor" "sys/sysmacros.h" HAS_MINOR_IN_SYSMACROS)
check_symbol_exists("makedev" "sys/sysmacros.h" HAS_MAKEDEV_IN_SYSMACROS)
if (HAS_MAJOR_IN_SYSMACROS AND HAS_MINOR_IN_SYSMACROS AND HAS_MAKEDEV_IN_SYSMACROS)
    set(MAJOR_IN_SYSMACROS 1)
endif ()

check_symbol_exists("major" "sys/mkdev.h" HAS_MAJOR_IN_MKDEV)
check_symbol_exists("minor" "sys/mkdev.h" HAS_MINOR_IN_MKDEV)
check_symbol_exists("makedev" "sys/mkdev.h" HAS_MAKEDEV_IN_MKDEV)
if (HAS_MAJOR_IN_MKDEV AND HAS_MINOR_IN_MKDEV AND HAS_MAKEDEV_IN_MKDEV)
    set(MAJOR_IN_MKDEV 1)
endif ()

check_function_exists("open_memstream" HAVE_OPEN_MEMSTREAM)
check_source_compiles(C "__attribute__((visibility(\"hidden\"))) int main(int, char**){return 0;};" HAVE_VISIBILITY)

set(DRM_CFLAGS "-D_GNU_SOURCE" "-fPIC")
foreach(flag HAVE_SYS_SYSCTL_H HAVE_SYS_SOCKET_H HAVE_ALLOCA_H MAJOR_IN_SYSMACROS MAJOR_IN_MKDEV HAVE_OPEN_MEMSTREAM HAVE_VISIBILITY)
    if (${flag})
        set(DRM_CFLAGS ${DRM_CFLAGS} "-D${flag}")
    endif ()
endforeach ()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/drm")
add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/drm/generated_static_table_fourcc.h"
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/drm/gen_table_fourcc.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/drm/include/drm/drm_fourcc.h"
            "${CMAKE_CURRENT_BINARY_DIR}/drm/generated_static_table_fourcc.h"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/xcb"
        COMMENT "Generating drm source code"
        VERBATIM
)

add_library(drm STATIC
        "drm/xf86drm.c"
        "drm/xf86drmHash.c"
        "drm/xf86drmRandom.c"
        "drm/xf86drmSL.c"
        "drm/xf86drmMode.c"
        "${CMAKE_CURRENT_BINARY_DIR}/drm/generated_static_table_fourcc.h")
target_include_directories(drm PRIVATE "drm/include/drm" "${CMAKE_CURRENT_BINARY_DIR}/drm")
target_compile_options(drm PRIVATE ${DRM_CFLAGS})