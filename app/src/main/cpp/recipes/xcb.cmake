file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/xcb")
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/xcb.h
        ${CMAKE_CURRENT_BINARY_DIR}/xcb/xcb.h)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/xcbext.h
        ${CMAKE_CURRENT_BINARY_DIR}/xcb/xcbext.h)

set (XCB_GENERATED)
set(codegen "${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/c_client.py")
set(site-packages "xcbproto")
file(GLOB xcbprotos "xcbproto/src/*.xml")

add_library(xcbproto INTERFACE)
target_include_directories(xcbproto INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/xcb" "libxcb/src")

foreach (proto IN LISTS xcbprotos)
    get_filename_component(name "${proto}" NAME)
    string(REGEX REPLACE "\\.xml$" ".c" gensrc "${CMAKE_CURRENT_BINARY_DIR}/xcb/${name}")
    string(REGEX REPLACE "\\.xml$" ".h" genhdr "${CMAKE_CURRENT_BINARY_DIR}/xcb/${name}")
    add_custom_command(
            OUTPUT "${gensrc}" "${genhdr}"
            COMMAND Python3::Interpreter "${codegen}" "-c" "libxcb 1.15"
                "-l" "X Version 11" "-s" "3" "-p" "${site-packages}" "${proto}"
            DEPENDS "${proto}" "${codegen_py}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/xcb"
            COMMENT "Generating source code from XML (${proto}) to ${gensrc} ${genhdr}"
            VERBATIM
    )
    set (XCB_GENERATED ${XCB_GENERATED} ${gensrc} ${genhdr})
endforeach ()
add_custom_target(xcb-generated-headers DEPENDS ${XCB_GENERATED})
add_dependencies(xcbproto xcb-generated-headers)

set_source_files_properties(${XCB_GENERATED} PROPERTIES GENERATED TRUE)

file(CONFIGURE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xcb-defs.c"
        CONTENT "
#include <unistd.h>

int asprintf(char **restrict strp, const char *restrict fmt, ...);
char *getenv(const char *name);

char *xcb_unix_base = NULL;

__attribute__((constructor))
static void init_path() {
    char* path;
    if (getenv(\"TMPDIR\"))
        path = getenv(\"TMPDIR\");
    else if (access(\"/tmp/\", F_OK) == 0)
        path = \"/tmp\";
    else if (access(\"/data/data/com.termux/files/\", F_OK) == 0)
        path = \"/data/data/com.termux/files/usr/tmp\";
    else if (access(\"/data/data/com.antutu.ABenchMark/files/image/\", F_OK) == 0)
        path = \"/data/data/com.antutu.ABenchMark/files/image/tmp\";
    else if (access(\"/data/data/com.ludashi.benchmark/files/image/\", F_OK) == 0)
        path = \"/data/data/com.ludashi.benchmark/files/image/tmp\";
    else if (access(\"/data/data/com.eltechs.ed/files/image/\", F_OK) == 0)
        path = \"/data/data/com.eltechs.ed/files/tmp/image/\";
    else
        path = \"/tmp\";

    asprintf(&xcb_unix_base, \"%s/.X11-unix/X\", path);
}
")

add_library(xcb STATIC
        "libxcb/src/xcb_auth.c"
        "libxcb/src/xcb_conn.c"
        "libxcb/src/xcb_ext.c"
        "libxcb/src/xcb_in.c"
        "libxcb/src/xcb_list.c"
        "libxcb/src/xcb_out.c"
        "libxcb/src/xcb_util.c"
        "libxcb/src/xcb_xid.c"
        "${CMAKE_CURRENT_BINARY_DIR}/xcb-defs.c"
        "${XCB_GENERATED}")
target_include_directories(xcb PRIVATE "libxcb/src/" "${CMAKE_CURRENT_BINARY_DIR}/xcb")
target_compile_options(xcb PRIVATE "-DXCB_QUEUE_BUFFER_SIZE=16384" "-DHAVE_SENDMSG" "-DIOV_MAX=16" "-fPIC")
target_link_libraries(xcb PUBLIC xcbproto Xau)

target_apply_patch(xcb "${CMAKE_CURRENT_SOURCE_DIR}/libxcb" "${CMAKE_CURRENT_SOURCE_DIR}/patches/xcb.patch")