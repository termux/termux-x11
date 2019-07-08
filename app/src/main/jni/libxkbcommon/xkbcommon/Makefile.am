ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xkbcommon.pc

EXTRA_DIST = \
	makekeys.py \
	src/xkbcomp/keywords.gperf \
	test/data \
	README.md \
	doc/quick-guide.md \
	doc/compat.md \
	doc/keymap-format-text-v1.txt \
	doc/rules-format.txt \
	doc/doxygen-extra.css \
	xkbcommon.map \
	xkbcommon-x11.map \
	PACKAGING

AM_CPPFLAGS = \
	-DDFLT_XKB_CONFIG_ROOT='"$(XKBCONFIGROOT)"' \
	-DXLOCALEDIR='"$(XLOCALEDIR)"' \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/xkbcomp \
	-I$(top_builddir)/src/xkbcomp \
	-include $(top_builddir)/src/config.h

AM_CFLAGS = $(BASE_CFLAGS)

AM_LDFLAGS = -no-undefined
if HAVE_NO_UNDEFINED
AM_LDFLAGS += -Wl,--no-undefined
endif

AM_YFLAGS = -d -p _xkbcommon_

xkbcommonincludedir = $(includedir)/xkbcommon
xkbcommoninclude_HEADERS = \
	xkbcommon/xkbcommon.h \
	xkbcommon/xkbcommon-compat.h \
	xkbcommon/xkbcommon-compose.h \
	xkbcommon/xkbcommon-keysyms.h \
	xkbcommon/xkbcommon-names.h

lib_LTLIBRARIES = libxkbcommon.la
libxkbcommon_la_SOURCES = \
	src/compose/parser.c \
	src/compose/parser.h \
	src/compose/paths.c \
	src/compose/paths.h \
	src/compose/state.c \
	src/compose/table.c \
	src/compose/table.h \
	src/xkbcomp/action.c \
	src/xkbcomp/action.h \
	src/xkbcomp/ast.h \
	src/xkbcomp/ast-build.c \
	src/xkbcomp/ast-build.h \
	src/xkbcomp/compat.c \
	src/xkbcomp/expr.c \
	src/xkbcomp/expr.h \
	src/xkbcomp/include.c \
	src/xkbcomp/include.h \
	src/xkbcomp/keycodes.c \
	src/xkbcomp/keymap.c \
	src/xkbcomp/keymap-dump.c \
	src/xkbcomp/keywords.c \
	src/xkbcomp/parser.y \
	src/xkbcomp/parser-priv.h \
	src/xkbcomp/rules.c \
	src/xkbcomp/rules.h \
	src/xkbcomp/scanner.c \
	src/xkbcomp/symbols.c \
	src/xkbcomp/types.c \
	src/xkbcomp/vmod.c \
	src/xkbcomp/vmod.h \
	src/xkbcomp/xkbcomp.c \
	src/xkbcomp/xkbcomp-priv.h \
	src/atom.c \
	src/atom.h \
	src/context.c \
	src/context.h \
	src/context-priv.c \
	src/darray.h \
	src/keysym.c \
	src/keysym.h \
	src/keysym-utf.c \
	src/ks_tables.h \
	src/keymap.c \
	src/keymap.h \
	src/keymap-priv.c \
	src/scanner-utils.h \
	src/state.c \
	src/text.c \
	src/text.h \
	src/utf8.c \
	src/utf8.h \
	src/utils.c \
	src/utils.h
libxkbcommon_la_LDFLAGS = -Wl,--version-script=${srcdir}/xkbcommon.map

if ENABLE_X11
pkgconfig_DATA += xkbcommon-x11.pc

xkbcommon_x11includedir = $(xkbcommonincludedir)
xkbcommon_x11include_HEADERS = \
	xkbcommon/xkbcommon-x11.h

lib_LTLIBRARIES += libxkbcommon-x11.la

libxkbcommon_x11_la_CFLAGS = $(AM_CFLAGS) $(XCB_XKB_CFLAGS)
libxkbcommon_x11_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/x11
libxkbcommon_x11_la_LIBADD = libxkbcommon.la $(XCB_XKB_LIBS)
libxkbcommon_x11_la_LDFLAGS = -Wl,--version-script=${srcdir}/xkbcommon-x11.map

libxkbcommon_x11_la_SOURCES = \
	src/x11/keymap.c \
	src/x11/state.c \
	src/x11/util.c \
	src/x11/x11-priv.h \
	src/context.h \
	src/context-priv.c \
	src/keymap.h \
	src/keymap-priv.c \
	src/atom.h \
	src/atom.c
endif ENABLE_X11

BUILT_SOURCES = \
	src/xkbcomp/parser.c \
	src/xkbcomp/parser.h
CLEANFILES = $(BUILT_SOURCES)

src/xkbcomp/parser.c: $(top_builddir)/src/$(am__dirstamp) $(top_builddir)/src/xkbcomp/$(am__dirstamp)
src/xkbcomp/parser.h: $(top_builddir)/src/$(am__dirstamp) $(top_builddir)/src/xkbcomp/$(am__dirstamp)

##
# Documentation
##

if ENABLE_DOCS
if HAVE_DOXYGEN
doc: doc/stamp-doxygen
clean-doc: clean-doxygen
all-local:: doc
clean-local:: clean-doc

doc/stamp-doxygen: $(top_srcdir)/xkbcommon/*.h
	$(AM_V_GEN)$(DOXYGEN) doc/Doxyfile
	touch $@

clean-doxygen:
	rm -rf doc/html doc/stamp-doxygen

install-data-local:: doc
	$(MKDIR_P) $(DESTDIR)$(htmldir)
	$(INSTALL_DATA) doc/html/* $(DESTDIR)$(htmldir)

uninstall-local::
	rm -rf $(DESTDIR)$(htmldir)
endif HAVE_DOXYGEN
endif ENABLE_DOCS

##
# Tests
##

# Some tests need to use unexported symbols, so we link them against
# a private copy of libxkbcommon with all symbols exposed.
check_LTLIBRARIES = libtest.la
libtest_la_SOURCES = \
	$(libxkbcommon_la_SOURCES) \
	test/common.c \
	test/test.h \
	test/evdev-scancodes.h

AM_TESTS_ENVIRONMENT = \
	XKB_LOG_LEVEL=debug; export XKB_LOG_LEVEL; \
	XKB_LOG_VERBOSITY=10; export XKB_LOG_VERBOSITY; \
	$(XORG_MALLOC_DEBUG_ENV)

TESTS = \
	test/keysym \
	test/filecomp \
	test/context \
	test/rules-file \
	test/stringcomp \
	test/buffercomp \
	test/log \
	test/atom \
	test/utf8 \
	test/state \
	test/keyseq \
	test/rulescomp \
	test/compose
check_PROGRAMS = \
	test/rmlvo-to-kccgst \
	test/print-compiled-keymap

TESTS_LDADD = libtest.la

test_keysym_LDADD = $(TESTS_LDADD)
test_filecomp_LDADD = $(TESTS_LDADD)
test_context_LDADD = $(TESTS_LDADD)
test_rules_file_CFLAGS = $(AM_CFLAGS) -Wno-declaration-after-statement
test_rules_file_LDADD = $(TESTS_LDADD)
test_stringcomp_LDADD = $(TESTS_LDADD)
test_buffercomp_LDADD = $(TESTS_LDADD)
test_log_LDADD = $(TESTS_LDADD)
test_atom_LDADD = $(TESTS_LDADD)
test_utf8_LDADD = $(TESTS_LDADD)
test_state_LDADD = $(TESTS_LDADD)
test_keyseq_LDADD = $(TESTS_LDADD)
test_rulescomp_LDADD = $(TESTS_LDADD)
test_rmlvo_to_kccgst_LDADD = $(TESTS_LDADD)
test_print_compiled_keymap_LDADD = $(TESTS_LDADD)
test_compose_LDADD = $(TESTS_LDADD) $(RT_LIBS)

if BUILD_LINUX_TESTS
check_PROGRAMS += \
	test/interactive-evdev

test_interactive_evdev_LDADD = $(TESTS_LDADD)
endif BUILD_LINUX_TESTS

if ENABLE_X11
TESTS += \
	test/x11 \
	test/x11comp
check_PROGRAMS += \
	test/interactive-x11

TESTS_X11_LDADD = $(XCB_XKB_LIBS) $(TESTS_LDADD) libxkbcommon-x11.la
TESTS_X11_CFLAGS = $(XCB_XKB_CFLAGS)

test_x11_LDADD = $(TESTS_X11_LDADD)
test_x11_CFLAGS = $(TESTS_X11_CFLAGS)
test_x11comp_LDADD = $(TESTS_X11_LDADD)
test_x11comp_CFLAGS = $(TESTS_X11_CFLAGS)
test_interactive_x11_LDADD = $(TESTS_X11_LDADD)
test_interactive_x11_CFLAGS = $(TESTS_X11_CFLAGS)
endif ENABLE_X11

check_PROGRAMS += $(TESTS)

##
# Benchmarks
##

BENCH_LDADD = $(TESTS_LDADD) $(RT_LIBS)

check_PROGRAMS += \
	bench/key-proc \
	bench/rules \
	bench/rulescomp \
	bench/compose
bench_key_proc_LDADD = $(BENCH_LDADD)
bench_rules_LDADD = $(BENCH_LDADD)
bench_rulescomp_LDADD = $(BENCH_LDADD)
bench_compose_LDADD = $(BENCH_LDADD)

##
# Custom targets
##

# This sed script strips out lines that start with '#define _' which
# removes #define _OSF_Keysyms and such.  The XK_Ydiaeresis case is to
# handle a duplicate definition in HPkeysyms.h which kicks in if it's
# not already defined.
X11_INCLUDEDIR = /usr/include/X11
KEYSYMDEFS = \
	 $(X11_INCLUDEDIR)/keysymdef.h \
	 $(X11_INCLUDEDIR)/XF86keysym.h \
	 $(X11_INCLUDEDIR)/Sunkeysym.h \
	 $(X11_INCLUDEDIR)/DECkeysym.h \
	 $(X11_INCLUDEDIR)/HPkeysym.h
update-keysyms:
	echo -en '#ifndef _XKBCOMMON_KEYSYMS_H\n#define _XKBCOMMON_KEYSYMS_H\n\n' > $(top_srcdir)/xkbcommon/xkbcommon-keysyms.h
	echo -en '/* This file is autogenerated from Makefile.am; please do not commit directly. */\n\n' >> $(top_srcdir)/xkbcommon/xkbcommon-keysyms.h
	echo -en '#define XKB_KEY_NoSymbol                    0x000000  /* Special KeySym */\n\n' >> $(top_srcdir)/xkbcommon/xkbcommon-keysyms.h
	sed -e '/XK_Ydiaeresis\s*0x100000ee/d' \
	    -e '/#define _/d' \
	    -e 's/#define\s*\(\w*\)XK_/#define XKB_KEY_\1/' \
	    -e '/\(#ifdef\|#ifndef\|#endif\)/d' $(KEYSYMDEFS) >> $(top_srcdir)/xkbcommon/xkbcommon-keysyms.h
	echo -en '\n\n#endif\n' >> $(top_srcdir)/xkbcommon/xkbcommon-keysyms.h
	LC_CTYPE=C python $(top_srcdir)/makekeys.py $(top_srcdir)/xkbcommon/xkbcommon-keysyms.h > $(top_srcdir)/src/ks_tables.h

# Run this if you add/remove a new keyword to the xkbcomp scanner,
# or just want to regenerate the gperf file.
update-keywords:
	$(AM_V_GEN)gperf < $(top_srcdir)/src/xkbcomp/keywords.gperf > $(top_srcdir)/src/xkbcomp/keywords.c

##
# Android stuff
##

Android_build.mk: Makefile $(BUILT_SOURCES)
	androgenizer \
	-:PROJECT libxkbcommon \
	-:REL_TOP $(top_srcdir) -:ABS_TOP $(abs_top_srcdir) \
	\
	-:STATIC libxkbcommon \
	-:TAGS eng debug \
	-:SOURCES $(filter-out %.l %.y,$(libxkbcommon_la_SOURCES)) $(BUILT_SOURCES) \
	-:CFLAGS $(DEFS) $(DEFAULT_INCLUDES) $(AM_CPPFLAGS) $(AM_CFLAGS) \
	-:LDFLAGS $(AM_LDFLAGS) \
	\
	-:PASSTHROUGH 'libxkbcommon-clean: clean-libxkbcommon' \
	> $@
