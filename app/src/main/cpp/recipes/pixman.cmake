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
#include <mmintrin.h>
#include <stdint.h>

/* Check support for block expressions */
#define _mm_shuffle_pi16(A, N)                    \
  ({                                              \
  __m64 ret;                                      \
                                                  \
  /* Some versions of clang will choke on K */    \
  asm (\"pshufw %2, %1, %0\n\t\"                  \
       :\"=y\" (ret)                              \
       : \"y\" (A), \"K\" ((const int8_t)N)       \
  );                                              \
                                                  \
  ret;                                            \
  })

int main () {
    __m64 v = _mm_cvtsi32_si64 (1);
    __m64 w;

    w = _mm_shuffle_pi16(v, 5);

    /* Some versions of clang will choke on this */
    asm (\"pmulhuw %1, %0\n\t\"
    : \"+y\" (w)
    : \"y\" (v)
    );

    return _mm_cvtsi64_si32 (v);
}" HAVE_MMX)
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
check_source_compiles(C "
#ifndef __IWMMXT__
#error \"IWMMXT not enabled (with -march=iwmmxt)\"
#endif
#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8))
#error \"Need GCC >= 4.8 for IWMMXT intrinsics\"
#endif
#include <mmintrin.h>
int main () {
    union {
        __m64 v;
        char c[8];
    } a = { .c = {1, 2, 3, 4, 5, 6, 7, 8} };
    int b = 4;
    __m64 c = _mm_srli_si64 (a.v, b);
}" HAVE_IWMMXT)
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS_OLD})
check_source_compiles(C "
int main () {
    /* Most modern architectures have a NOP instruction, so this is a fairly generic test. */
    asm volatile ( \"\tnop\n\" : : : \"cc\", \"memory\" );
    return 0;
}" HAVE_GNU_INLINE_ASM)
try_compile(HAVE_SIMD ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/pixman/arm-simd-test.S)
try_compile(HAVE_NEON ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/pixman/neon-test.S)
try_compile(HAVE_A64_NEON ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/pixman/a64-neon-test.S)

check_type_size("long" SIZEOF_LONG)

add_library(pixman STATIC
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
target_compile_definitions(pixman PRIVATE
        "-DHAVE_BUILTIN_CLZ=1"
        "-DHAVE_PTHREADS=1"
        "-DPACKAGE=\"pixman\""
        "-DTLS=__thread"
        "-DSIZEOF_LONG=${SIZEOF_LONG}"
        "-DUSE_OPENMP=1")
target_include_directories(pixman PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

if (HAVE_MMX)
    target_compile_options(pixman PRIVATE "-mmmx" "-Winline")
endif()
if (HAVE_SSE2)
    target_compile_options(pixman PRIVATE "-msse2" "-Winline")
endif()
if (HAVE_SSSE3)
    target_compile_options(pixman PRIVATE "-mssse3" "-Winline")
endif()
if (HAVE_SIMD)
    add_library(pixman-simd STATIC "pixman/pixman/pixman-arm-simd-asm.S" "pixman/pixman/pixman-arm-simd-asm-scaled.S")
    target_link_libraries(pixman PRIVATE pixman-simd)
endif()
if (HAVE_NEON)
    add_library(pixman-neon STATIC "pixman/pixman/pixman-arm-neon-asm.S" "pixman-arm-neon-asm-bilinear.S")
    target_link_libraries(pixman PRIVATE pixman-neon)
endif()
if (HAVE_A64_NEON)
    add_library(pixman-a64-neon STATIC "pixman/pixman/pixman-arma64-neon-asm.S" "pixman-arma64-neon-asm-bilinear.S")
    target_link_libraries(pixman PRIVATE pixman-a64-neon)
endif()
