cmake_minimum_required(VERSION 3.22)
project(lorie)

set(can_use_assembler TRUE)
enable_language(ASM)

#set_property(TARGET tgt PROPERTY C_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

include(TestBigEndian)
include(CheckIncludeFile)
include(CheckSourceCompiles)
include(CheckCCompilerFlag)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckTypeSize)

find_package(Python3 REQUIRED)
find_package(BISON REQUIRED)

function(target_apply_patch patch_target source_dir patch_file)
    string(RANDOM LENGTH 32 temp_name)
    add_custom_target(${patch_target}_${temp_name}
            COMMAND
                "patch" "-p1" "-d" "${source_dir}" "-i" "${patch_file}" "-R" "--dry-run" ">" "/dev/null" "2>&1" "||"
                "patch" "-p1" "-N" "-d" "${source_dir}" "-i" "${patch_file}" "-r" "-"
            COMMENT "Applying patch ${patch}"
            VERBATIM
    )
    add_dependencies(${patch_target} ${patch_target}_${temp_name})
    set(temp_name)
endfunction()

set(test_compile_options
        "-Wall"
        "-Wpointer-arith"
        "-Wmissing-declarations"
        "-Wformat=2"
        "-Winvalid-pch"
        "-Wstrict-prototypes"
        "-Wmissing-prototypes"
        "-Wnested-externs"
        "-Wbad-function-cast"
        "-Wold-style-definition"
        "-Wdeclaration-after-statement"
        "-Wunused"
        "-Wuninitialized"
        "-Wshadow"
        "-Wmissing-noreturn"
        "-Wmissing-format-attribute"
        "-Wredundant-decls"
        "-Wlogical-op"
        "-Werror=implicit"
        "-Werror=nonnull"
        "-Werror=init-self"
        "-Werror=main"
        "-Werror=missing-braces"
        "-Werror=sequence-point"
        "-Werror=return-type"
        "-Werror=trigraphs"
        "-Werror=array-bounds"
        "-Werror=write-strings"
        "-Werror=address"
        "-Werror=int-to-pointer-cast"
        "-Werror=pointer-to-int-cast"
        "-fno-strict-aliasing"
        "-fsanitize=address")

set(CMAKE_REQUIRED_QUIET_OLD ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET 1)
set(common_compile_options)
set(I 0)
foreach (option ${test_compile_options})
    check_c_compiler_flag(${option} COMPILER_TEST_${I})
    if (COMPILER_TEST_${I})
        set(common_compile_options ${common_compile_options} ${option})
    endif()
    MATH(EXPR I "${I}+1")
endforeach ()
set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_OLD})

foreach(project xorgproto fontenc md drm freetype pixman tirpc xshmfence xcb Xext Xfixes Xxf86vm xkbcommon
                xcb-errors X11 Xtrans xkbfile Xdmcp Xau Xfont2 xkbcomp mesa xserver epoxy virglrenderer)
    include(recipes/${project}.cmake)
endforeach()

add_library(android-shmem STATIC "lorie/shm/shmem.c")
target_compile_options(android-shmem
        PRIVATE "-D_PATH_TMP=(getenv(\"TMPDIR\") ?: \"/tmp\")"
        PUBLIC "-include" "${CMAKE_CURRENT_SOURCE_DIR}/lorie/shm/shm.h")
