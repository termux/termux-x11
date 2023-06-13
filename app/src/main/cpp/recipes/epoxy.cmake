file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/epoxy")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/epoxy/GL")
file(CONFIGURE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/epoxy/config.h"
        CONTENT "\
#pragma once
#define ENABLE_EGL 1
#define EPOXY_PUBLIC __attribute__((visibility(\"default\"))) extern
#define HAVE_KHRPLATFORM_H
")
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/epoxy/GL/gl.h" CONTENT "#include <epoxy/gl.h>")
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/epoxy/GL/glext.h" CONTENT "")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/epoxy/gl_generated_dispatch.c"
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/libepoxy/src/gen_dispatch.py" "--source" "--no-header"
        "--outputdir=${CMAKE_CURRENT_BINARY_DIR}/epoxy" "${CMAKE_CURRENT_SOURCE_DIR}/libepoxy/registry/gl.xml"
        COMMENT "Generating source code (gl_generated_dispatch.c)"
        VERBATIM)
add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/epoxy/gl_generated.h"
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/libepoxy/src/gen_dispatch.py" "--header" "--no-source"
            "--outputdir=${CMAKE_CURRENT_BINARY_DIR}/epoxy" "${CMAKE_CURRENT_SOURCE_DIR}/libepoxy/registry/gl.xml"
        COMMENT "Generating source code (gl_generated.h)"
        VERBATIM)
add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/epoxy/egl_generated_dispatch.c"
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/libepoxy/src/gen_dispatch.py" "--source" "--no-header"
            "--outputdir=${CMAKE_CURRENT_BINARY_DIR}/epoxy" "${CMAKE_CURRENT_SOURCE_DIR}/libepoxy/registry/egl.xml"
        COMMENT "Generating source code (egl_generated_dispatch.c)"
        VERBATIM)
add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/epoxy/egl_generated.h"
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/libepoxy/src/gen_dispatch.py" "--header" "--no-source"
            "--outputdir=${CMAKE_CURRENT_BINARY_DIR}/epoxy" "${CMAKE_CURRENT_SOURCE_DIR}/libepoxy/registry/egl.xml"
        COMMENT "Generating source code (egl_generated.h)"
        VERBATIM)

add_library(epoxy STATIC
        "libepoxy/src/dispatch_common.c"
        "libepoxy/src/dispatch_egl.c"
        "${CMAKE_CURRENT_BINARY_DIR}/epoxy/gl_generated_dispatch.c"
        "${CMAKE_CURRENT_BINARY_DIR}/epoxy/gl_generated.h"
        "${CMAKE_CURRENT_BINARY_DIR}/epoxy/egl_generated_dispatch.c"
        "${CMAKE_CURRENT_BINARY_DIR}/epoxy/egl_generated.h")
target_include_directories(epoxy PUBLIC "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/epoxy" "libepoxy/src" "libepoxy/include")
