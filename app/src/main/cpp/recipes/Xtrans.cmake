file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Xtrans/X11/Xtrans")
foreach(src Xtrans.h Xtrans.c Xtransint.h Xtranslcl.c Xtranssock.c Xtransutil.c transport.c)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_CURRENT_SOURCE_DIR}/libxtrans/${src}"
            "${CMAKE_CURRENT_BINARY_DIR}/Xtrans/X11/Xtrans/${src}")
endforeach()

file(CONFIGURE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/X11/Xtrans-defs.c"
        CONTENT "
#include <unistd.h>

int asprintf(char **restrict strp, const char *restrict fmt, ...);
char *getenv(const char *name);

char *xtrans_unix_path_x11 = NULL;
char *xtrans_unix_dir_x11 = NULL;
char *xtrans_unix_path_xim = NULL;
char *xtrans_unix_dir_xim = NULL;
char *xtrans_unix_path_fs = NULL;
char *xtrans_unix_dir_fs = NULL;
char *xtrans_unix_path_ice = NULL;
char *xtrans_unix_dir_ice = NULL;

__attribute__((constructor))
static void init_pathes() {
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
        path = \"/data/data/com.eltechs.ed/files/image/tmp\";
    else
        path = \"/tmp\";

    asprintf(&xtrans_unix_path_x11, \"%s/.X11-unix/X\", path);
    asprintf(&xtrans_unix_dir_x11, \"%s/.X11-unix/\", path);
    asprintf(&xtrans_unix_path_xim, \"%s/.XIM-unix/XIM\", path);
    asprintf(&xtrans_unix_dir_xim, \"%s/.XIM-unix\", path);
    asprintf(&xtrans_unix_path_fs, \"%s/.font-unix/fs\", path);
    asprintf(&xtrans_unix_dir_fs, \"%s/.font-unix\", path);
    asprintf(&xtrans_unix_path_ice, \"%s/.ICE-unix/\", path);
    asprintf(&xtrans_unix_dir_ice, \"%s/.ICE-unix\", path);
}
")

add_library(Xtrans STATIC "${CMAKE_CURRENT_BINARY_DIR}/X11/Xtrans-defs.c")
target_compile_options(Xtrans PRIVATE "-fPIC")
target_include_directories(Xtrans INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/Xtrans)

set(d ${CMAKE_CURRENT_SOURCE_DIR}/libxtrans)
set(p ${CMAKE_CURRENT_SOURCE_DIR}/patches/Xtrans.patch)
execute_process(COMMAND "bash" "-c" "patch -p1 -d ${d} -i ${p} -R --dry-run > /dev/null 2>&1 || patch -p1 -N -d ${d} -i ${p} -r -")
unset(d)
unset(p)
