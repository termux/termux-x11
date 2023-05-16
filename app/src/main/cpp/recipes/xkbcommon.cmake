file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/xkbcommon")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/xkbcommon/config.h "")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/xkbcommon/config.c
        "#include <keymap.h>
  const struct xkb_keymap_format_ops text_v1_keymap_format_ops = {};")
set(XKBCOMMON_SOURCES
        atom.c context.c context-priv.c keysym.c keysym-utf.c keymap.c keymap-priv.c state.c text.c
        utf8.c utils.c x11/keymap.c x11/state.c x11/util.c)
list(TRANSFORM XKBCOMMON_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/libxkbcommon/src/)
set(XKBCOMMON_SOURCES ${XKBCOMMON_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/xkbcommon/config.c)
add_library(xkbcommon STATIC ${XKBCOMMON_SOURCES})
target_compile_options(xkbcommon PRIVATE
        "-DHAVE_STRNDUP=1" "-DDEFAULT_XKB_LAYOUT=\"us\"" "-DDEFAULT_XKB_MODEL=\"pc105\""
        "-DDEFAULT_XKB_OPTIONS=NULL" "-DDEFAULT_XKB_RULES=\"evdev\"" "-DDEFAULT_XKB_VARIANT=NULL"
        "-DDFLT_XKB_CONFIG_EXTRA_PATH=\"/data/data/com.termux.x11\""
        "-DDFLT_XKB_CONFIG_ROOT=\"/data/data/com.termux.x11\"")
target_include_directories(xkbcommon PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/xkbcommon"
        "${CMAKE_CURRENT_SOURCE_DIR}/libxkbcommon/src")
target_include_directories(xkbcommon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libxkbcommon/include")
target_link_libraries(xkbcommon xcbproto)
