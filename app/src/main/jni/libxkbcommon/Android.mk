LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := xkbcommon
LOCAL_SRC_FILES := \
	xkbcommon/src/compose/parser.c \
	xkbcommon/src/compose/paths.c \
	xkbcommon/src/compose/state.c \
	xkbcommon/src/compose/table.c \
	xkbcommon/src/xkbcomp/action.c \
	xkbcommon/src/xkbcomp/ast-build.c \
	xkbcommon/src/xkbcomp/compat.c \
	xkbcommon/src/xkbcomp/expr.c \
	xkbcommon/src/xkbcomp/include.c \
	xkbcommon/src/xkbcomp/keycodes.c \
	xkbcommon/src/xkbcomp/keymap.c \
	xkbcommon/src/xkbcomp/keymap-dump.c \
	xkbcommon/src/xkbcomp/keywords.c \
	xkbcommon/src/xkbcomp/parser.c \
	xkbcommon/src/xkbcomp/rules.c \
	xkbcommon/src/xkbcomp/scanner.c \
	xkbcommon/src/xkbcomp/symbols.c \
	xkbcommon/src/xkbcomp/types.c \
	xkbcommon/src/xkbcomp/vmod.c \
	xkbcommon/src/xkbcomp/xkbcomp.c \
	xkbcommon/src/atom.c \
	xkbcommon/src/context.c \
	xkbcommon/src/context-priv.c \
	xkbcommon/src/keysym.c \
	xkbcommon/src/keysym-utf.c \
	xkbcommon/src/keymap.c \
	xkbcommon/src/keymap-priv.c \
	xkbcommon/src/state.c \
	xkbcommon/src/text.c \
	xkbcommon/src/utf8.c \
	xkbcommon/src/utils.c
LOCAL_CFLAGS := \
	-std=c99 -Wall -Werror -Wno-unused-parameter -Wno-missing-field-initializers \
	-D_GNU_SOURCE \
	-DXLOCALEDIR=\"/data/data/com.termux/files/usr/share/X11/locale\" \
	-DDEFAULT_XKB_LAYOUT=\"us\" \
	-DDEFAULT_XKB_MODEL=\"pc105\" \
	-DDEFAULT_XKB_RULES=\"evdev\" \
	-DDFLT_XKB_CONFIG_ROOT=\"/data/data/com.termux/files/usr/share/X11/xkb\"
LOCAL_C_INCLUDES := $(LOCAL_PATH)/xkbcommon $(LOCAL_PATH)/xkbcommon/src
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/xkbcommon/
include $(BUILD_SHARED_LIBRARY)
