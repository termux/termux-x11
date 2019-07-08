dnl  Copyright © 2008 Dan Nicholson
dnl
dnl  Permission to use, copy, modify, distribute, and sell this software and its
dnl  documentation for any purpose is hereby granted without fee, provided that
dnl  the above copyright notice appear in all copies and that both that
dnl  copyright notice and this permission notice appear in supporting
dnl  documentation, and that the name of Keith Packard not be used in
dnl  advertising or publicity pertaining to distribution of the software without
dnl  specific, written prior permission.  Keith Packard makes no
dnl  representations about the suitability of this software for any purpose.  It
dnl  is provided "as is" without express or implied warranty.
dnl
dnl  DAN NICHOLSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
dnl  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
dnl  EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
dnl  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
dnl  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
dnl  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
dnl  PERFORMANCE OF THIS SOFTWARE.
dnl
dnl Process this file with autoconf to create configure.

# Initialize Autoconf
AC_PREREQ([2.62])
AC_INIT([libxkbcommon], [0.5.0],
        [https://bugs.freedesktop.org/enter_bug.cgi?product=libxkbcommon],
        [libxkbcommon], [http://xkbcommon.org])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-xz no-dist-gzip subdir-objects color-tests parallel-tests check-news])
AM_MAINTAINER_MODE([enable])

# Get _GNU_SOURCE and friends
AC_USE_SYSTEM_EXTENSIONS

# Initialize libtool
LT_INIT

# Add xorg-macros stuff
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.16 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.16)
XORG_DEFAULT_OPTIONS
XORG_MEMORY_CHECK_FLAGS
XORG_ENABLE_DOCS
XORG_WITH_DOXYGEN

# Needed in older Automakes for subdir-objects
AM_PROG_CC_C_O

# Check for compiler features
AC_C_INLINE

# Check for programs
AC_PROG_MKDIR_P
PKG_PROG_PKG_CONFIG

# Note: we use some yacc extensions, which work with either GNU bison
# (preferred) or byacc. Other yacc's may or may not work.
AC_PROG_YACC
AC_PATH_PROG([YACC_INST], [$YACC])
AS_IF([test ! -f "src/xkbcomp/parser.c" -a "x$YACC_INST" = x], [
    AC_MSG_ERROR([yacc not found - unable to compile src/xkbcomp/parser.y])
])

# Checks for library functions.
AC_CHECK_FUNCS([strcasecmp strncasecmp])
AS_IF([test "x$ac_cv_func_strcasecmp" = xno -o \
            "x$ac_cv_func_strncasecmp" = xno], [
    AC_MSG_ERROR([C library does not support strcasecmp/strncasecmp])
])

AC_CHECK_FUNCS([eaccess euidaccess mmap])

AC_CHECK_FUNCS([secure_getenv __secure_getenv])
AS_IF([test "x$ac_cv_func_secure_getenv" = xno -a \
            "x$ac_cv_func___secure_getenv" = xno], [
    AC_MSG_WARN([C library does not support secure_getenv, using getenv instead])
])

AX_GCC_BUILTIN(__builtin_expect)

# Some tests use Linux-specific headers
AC_CHECK_HEADER([linux/input.h])
AM_CONDITIONAL(BUILD_LINUX_TESTS, [test "x$ac_cv_header_linux_input_h" = xyes])

XORG_TESTSET_CFLAG([BASE_CFLAGS], [-fvisibility=hidden])
XORG_TESTSET_CFLAG([BASE_CFLAGS], [-Wextra -Wno-unused-parameter -Wno-missing-field-initializers])
XORG_TESTSET_CFLAG([BASE_CFLAGS], [-Wdocumentation])

# OpenBSD does not have DT_NEEDED entries for libc by design
# so when these flags are passed to ld via libtool the checks will fail
case "$host_os" in
openbsd*)
    ;;
*)
    XORG_CHECK_LINKER_FLAGS([-Wl,--no-undefined], [have_no_undefined=yes]) ;;
esac

AM_CONDITIONAL([HAVE_NO_UNDEFINED], [test "x$have_no_undefined" = xyes])

AC_CHECK_LIB(rt, clock_gettime,
    [AC_SUBST(RT_LIBS, "-lrt")],
    [AC_SUBST(RT_LIBS, "")],
    [-lrt])

# Define a configuration option for the XKB config root
xkb_base=`$PKG_CONFIG --variable=xkb_base xkeyboard-config`
AS_IF([test "x$xkb_base" = x], [
    xkb_base="$datadir/X11/xkb"
])
AC_ARG_WITH([xkb_config_root],
    [AS_HELP_STRING([--with-xkb-config-root=<path>],
        [Set default XKB config root (default: xkeyboard-config install path)])],
    [XKBCONFIGROOT="$withval"],
    [XKBCONFIGROOT="$xkb_base"])
AC_SUBST([XKBCONFIGROOT])

# Define a configuration option for the X locale directory for compose
AC_ARG_WITH([x_locale_root],
    [AS_HELP_STRING([--with-x-locale-root=<path>],
        [Set X locale root (default: $datadir/X11/locale)])],
    [XLOCALEDIR="$withval"],
    [XLOCALEDIR="$datadir/X11/locale"])
AC_SUBST([XLOCALEDIR])

AC_ARG_WITH([default_rules],
    [AS_HELP_STRING([--with-default-rules=<path>],
        [Default XKB ruleset (default: evdev)])],
    [DEFAULT_XKB_RULES="$withval"],
    [DEFAULT_XKB_RULES="evdev"])
AC_DEFINE_UNQUOTED([DEFAULT_XKB_RULES], ["$DEFAULT_XKB_RULES"],
                   [Default XKB ruleset])

AC_ARG_WITH([default_model],
    [AS_HELP_STRING([--with-default-model=<path>],
        [Default XKB model (default: pc105)])],
    [DEFAULT_XKB_MODEL="$withval"],
    [DEFAULT_XKB_MODEL="pc105"])
AC_DEFINE_UNQUOTED([DEFAULT_XKB_MODEL], ["$DEFAULT_XKB_MODEL"],
                   [Default XKB model])

AC_ARG_WITH([default_layout],
    [AS_HELP_STRING([--with-default-layout=<path>],
        [Default XKB layout (default: us)])],
    [DEFAULT_XKB_LAYOUT="$withval"],
    [DEFAULT_XKB_LAYOUT="us"])
AC_DEFINE_UNQUOTED([DEFAULT_XKB_LAYOUT], ["$DEFAULT_XKB_LAYOUT"],
                   [Default XKB layout])

AC_ARG_WITH([default_variant],
    [AS_HELP_STRING([--with-default-variant=<path>],
        [Default XKB variant (default: (none))])],
    [DEFAULT_XKB_VARIANT="$withval"],
    [DEFAULT_XKB_VARIANT=])
AS_IF([test "x$DEFAULT_XKB_VARIANT" != x], [
    AC_DEFINE_UNQUOTED([DEFAULT_XKB_VARIANT], ["$DEFAULT_XKB_VARIANT"],
                       [Default XKB variant])
])

AC_ARG_WITH([default_options],
    [AS_HELP_STRING([--with-default-options=<path>],
        [Default XKB options (default: (none))])],
    [DEFAULT_XKB_OPTIONS="$withval"],
    [DEFAULT_XKB_OPTIONS=])
AS_IF([test "x$DEFAULT_XKB_OPTIONS" != x], [
    AC_DEFINE_UNQUOTED([DEFAULT_XKB_OPTIONS], ["$DEFAULT_XKB_OPTIONS"],
                       [Default XKB options])
])

AC_ARG_ENABLE([x11],
    [AS_HELP_STRING([--disable-x11],
        [Disable support for creating keymaps with the X11 protocol (default: enabled)])],
    [], [enable_x11=yes])
AS_IF([test "x$enable_x11" = xyes], [
    PKG_CHECK_MODULES([XCB_XKB], [xcb xcb-xkb >= 1.10], [],
        [AC_MSG_ERROR([xkbcommon-x11 requires xcb-xkb >= 1.10 which was not found. \
You can disable X11 support with --disable-x11.])])
], [enable_x11=no])
AM_CONDITIONAL([ENABLE_X11], [test "x$enable_x11" = xyes])

AC_CONFIG_FILES([
    Makefile
    xkbcommon-uninstalled.pc
    xkbcommon.pc
    xkbcommon-x11.pc
    xkbcommon-x11-uninstalled.pc
    doc/Doxyfile
])
AC_OUTPUT

AC_MSG_RESULT([
        $PACKAGE_NAME $VERSION

        libxkbcommon:            yes
        libxkbcommon-x11:        ${enable_x11}
        documentation:           ${build_docs}

        default XKB rules:       ${DEFAULT_XKB_RULES}
        default XKB model:       ${DEFAULT_XKB_MODEL}
        default XKB layout:      ${DEFAULT_XKB_LAYOUT}
        default XKB variant:     ${DEFAULT_XKB_VARIANT}
        default XKB options:     ${DEFAULT_XKB_OPTIONS}

        prefix:                  ${prefix}
        includedir:              ${includedir}
        lib dir:                 ${libdir}
        XKB config root:         ${XKBCONFIGROOT}
        X11 locale root:         ${XLOCALEDIR}
])
