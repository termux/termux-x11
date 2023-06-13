file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pixman-version.h"
        CONTENT "
#pragma once
#ifndef PIXMAN_H__
#  error pixman-version.h should only be included by pixman.h
#endif

#define PIXMAN_VERSION_MAJOR 0
#define PIXMAN_VERSION_MINOR 42
#define PIXMAN_VERSION_MICRO 2

#define PIXMAN_VERSION_STRING \"0.42.2\"

#define PIXMAN_VERSION_ENCODE(major, minor, micro) (	\
	                      ((major) * 10000)				\
                    	+ ((minor) *   100)				\
	                    + ((micro) *     1))

#define PIXMAN_VERSION PIXMAN_VERSION_ENCODE(	\
	PIXMAN_VERSION_MAJOR,			\
	PIXMAN_VERSION_MINOR,			\
	PIXMAN_VERSION_MICRO)

#ifndef PIXMAN_API
# define PIXMAN_API
#endif
")

check_source_compiles(C "
#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 2))
#   if !defined(__amd64__) && !defined(__x86_64__)
#      error \"Need GCC >= 4.2 for SSE2 intrinsics on x86\"
#   endif
#endif
#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
int param;
int main () {
    __m128i a = _mm_set1_epi32 (param), b = _mm_set1_epi32 (param + 1), c;
    c = _mm_xor_si128 (a, b);
    return _mm_cvtsi128_si32(c);
}" HAVE_SSE2)
check_source_compiles(C "
#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
int param;
int main () {
    __m128i a = _mm_set1_epi32 (param), b = _mm_set1_epi32 (param + 1), c;
    c = _mm_xor_si128 (a, b);
    return _mm_cvtsi128_si32(c);
}" HAVE_SSSE3)
set(CMAKE_REQUIRED_DEFINITIONS_OLD ${CMAKE_REQUIRED_DEFINITIONS})
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} "-march=iwmmxt2")
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS_OLD})
check_source_compiles(C "
.text
.arch armv6
.object_arch armv4
.arm
.altmacro
#ifndef __ARM_EABI__
#error EABI is required (to be sure that calling conventions are compatible)
#endif
pld [r0]
uqadd8 r0, r0, r0

.global main
main:
" HAVE_SIMD SRC_EXT S)
check_source_compiles(C "
.text
.fpu neon
.arch armv7a
.object_arch armv4
.eabi_attribute 10, 0
.arm
.altmacro
#ifndef __ARM_EABI__
#error EABI is required (to be sure that calling conventions are compatible)
#endif
pld [r0]
vmovn.u16 d0, q0

.global main
main:
" HAVE_NEON SRC_EXT S)
check_source_compiles(C "
.text
.arch armv8-a
.altmacro
prfm pldl2strm, [x0]
xtn v0.8b, v0.8h

.global main
main:
" HAVE_A64_NEON SRC_EXT S)
check_type_size("long" SIZEOF_LONG)

#message(FATAL_ERROR "HAVE_SSE2 ${HAVE_SSE2} HAVE_SSSE3 ${HAVE_SSSE3} HAVE_SIMD ${HAVE_SIMD} HAVE_NEON ${HAVE_NEON} HAVE_A64_NEON ${HAVE_A64_NEON}")

set(PIXMAN_SRC
        pixman/pixman/pixman.c
        pixman/pixman/pixman-access.c
        pixman/pixman/pixman-access-accessors.c
        pixman/pixman/pixman-bits-image.c
        pixman/pixman/pixman-combine32.c
        pixman/pixman/pixman-combine-float.c
        pixman/pixman/pixman-conical-gradient.c
        pixman/pixman/pixman-filter.c
        pixman/pixman/pixman-x86.c
        pixman/pixman/pixman-mips.c
        pixman/pixman/pixman-arm.c
        pixman/pixman/pixman-ppc.c
        pixman/pixman/pixman-edge.c
        pixman/pixman/pixman-edge-accessors.c
        pixman/pixman/pixman-fast-path.c
        pixman/pixman/pixman-glyph.c
        pixman/pixman/pixman-general.c
        pixman/pixman/pixman-gradient-walker.c
        pixman/pixman/pixman-image.c
        pixman/pixman/pixman-implementation.c
        pixman/pixman/pixman-linear-gradient.c
        pixman/pixman/pixman-matrix.c
        pixman/pixman/pixman-noop.c
        pixman/pixman/pixman-radial-gradient.c
        pixman/pixman/pixman-region16.c
        pixman/pixman/pixman-region32.c
        pixman/pixman/pixman-solid-fill.c
        pixman/pixman/pixman-timer.c
        pixman/pixman/pixman-trap.c
        pixman/pixman/pixman-utils.c)

set(PIXMAN_CFLAGS
        "-DHAVE_BUILTIN_CLZ=1"
        "-DHAVE_PTHREADS=1"
        "-DPACKAGE=\"pixman\""
        "-DTLS=__thread"
        "-DSIZEOF_LONG=${SIZEOF_LONG}"
        "-DUSE_OPENMP=1")

if (HAVE_SSE2)
    set(PIXMAN_CFLAGS ${PIXMAN_CFLAGS}  "-msse2" "-Winline")
endif()
if (HAVE_SSSE3)
    set(PIXMAN_CFLAGS ${PIXMAN_CFLAGS}  "-mssse3" "-Winline")
endif()

if (HAVE_NEON OR HAVE_A64_NEON)
    set(PIXMAN_SRC ${PIXMAN_SRC} "pixman/pixman/pixman-arm-neon.c")
endif()
if (HAVE_SIMD)
    set(PIXMAN_SRC ${PIXMAN_SRC}
            "pixman/pixman/pixman-arm-simd-asm.S"
            "pixman/pixman/pixman-arm-simd-asm-scaled.S")
    set(PIXMAN_CFLAGS ${PIXMAN_CFLAGS}  "-DUSE_ARM_SIMD=1")
endif()
if (HAVE_NEON)
    set(PIXMAN_SRC ${PIXMAN_SRC}
            "pixman/pixman/pixman-arm-neon-asm.S"
            "pixman/pixman/pixman-arm-neon-asm-bilinear.S")
    set(PIXMAN_CFLAGS ${PIXMAN_CFLAGS}  "-DUSE_ARM_NEON=1" "-fno-integrated-as" "-B" "/usr/arm-linux-gnueabihf/bin" "-v")
endif()
if (HAVE_A64_NEON)
    set(PIXMAN_SRC ${PIXMAN_SRC}
            "pixman/pixman/pixman-arma64-neon-asm.S"
            "pixman/pixman/pixman-arma64-neon-asm-bilinear.S")
    set(PIXMAN_CFLAGS ${PIXMAN_CFLAGS}  "-DUSE_ARM_A64_NEON=1" "-fno-integrated-as" "-B" "/usr/aarch64-linux-gnu/bin")
endif()

add_library(pixman STATIC ${PIXMAN_SRC})
target_compile_options(pixman PRIVATE ${PIXMAN_CFLAGS})
target_include_directories(pixman PRIVATE pixman "${CMAKE_CURRENT_BINARY_DIR}")
target_apply_patch(pixman "${CMAKE_CURRENT_SOURCE_DIR}/pixman" "${CMAKE_CURRENT_SOURCE_DIR}/patches/pixman.patch")