cmake_minimum_required(VERSION 3.22)
project(lorie)
set(CMAKE_CXX_STANDARD 20)

find_package(Python3 REQUIRED)

# Nothing special here, only generating xcb headers and building xcb...

####################################################################################################
######################################### XCB PROTOS ###############################################
####################################################################################################

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/generated ${CMAKE_CURRENT_BINARY_DIR}/generated/xcb)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/xcb.h ${CMAKE_CURRENT_BINARY_DIR}/generated/xcb.h)

set (XCB_GENERATED)
set(codegen "${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/c_client.py")
set(site-packages "${CMAKE_CURRENT_SOURCE_DIR}/xcbproto")
file(GLOB xcbprotos "${CMAKE_CURRENT_SOURCE_DIR}/xcbproto/src/*.xml")

add_library(xcbproto INTERFACE)
target_include_directories(xcbproto INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated/ ${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src)

foreach (proto IN LISTS xcbprotos)
    get_filename_component(name "${proto}" NAME)
    string(REGEX REPLACE "\\.xml$" ".c" gensrc "${CMAKE_CURRENT_BINARY_DIR}/generated/${name}")
    string(REGEX REPLACE "\\.xml$" ".h" genhdr "${CMAKE_CURRENT_BINARY_DIR}/generated/${name}")
    add_custom_command(
            OUTPUT "${gensrc}" "${genhdr}"
            COMMAND Python3::Interpreter "${codegen}" "-c" "libxcb 1.15"
            "-l" "X Version 11" "-s" "3" "-p" "${site-packages}" "${proto}"
            DEPENDS "${header}" "${codegen_py}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated"
            COMMENT "Generating source code from XML (${proto}) to ${gensrc} ${genhdr}"
            VERBATIM
    )
    set (XCB_GENERATED ${XCB_GENERATED} ${gensrc} ${genhdr})
endforeach ()
add_custom_target(xcb-generated-headers DEPENDS ${XCB_GENERATED})
add_dependencies(xcbproto xcb-generated-headers)

set_source_files_properties(${XCB_GENERATED} PROPERTIES GENERATED TRUE)

####################################################################################################
######################################### XORGPROTO ################################################
####################################################################################################

add_library(xorgproto INTERFACE)
target_include_directories(xorgproto INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/xorgproto/include)

####################################################################################################
########################################### LIBXAU #################################################
####################################################################################################

set(XAU_SOURCES AuDispose.c AuFileName.c AuGetAddr.c AuGetBest.c AuLock.c AuRead.c Autest.c
        AuUnlock.c AuWrite.c include/X11/Xauth.h)
list(TRANSFORM XAU_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/libXau/)
add_library(Xau STATIC ${XAU_SOURCES})
target_include_directories(Xau PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libXau/include)
target_link_libraries(Xau xorgproto xcbproto)

####################################################################################################
########################################### LIBXCB #################################################
####################################################################################################

set(XCB_SOURCES xcb_auth.c xcb_conn.c xcb_ext.c xcb_in.c xcb_list.c
        xcb_out.c xcb_util.c xcb_xid.c xcb.h xcbext.h xcbint.h xcb_windefs.h)
list(TRANSFORM XCB_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/)
add_library(xcb STATIC ${XCB_SOURCES} ${XCB_GENERATED})
target_include_directories(xcb PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/" "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_options(xcb PRIVATE "-DXCB_QUEUE_BUFFER_SIZE=16384" "-DHAVE_SENDMSG")
target_link_libraries(xcb Xau)

####################################################################################################
####################################### LIBXCB-ERRORS ##############################################
####################################################################################################

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/extensions.c"
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/libxcb-errors/src/extensions.py"
        "${CMAKE_CURRENT_BINARY_DIR}/extensions.c" ${xcbprotos}
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libxcb-errors/src/extensions.py" ${xcbprotos}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating errors database from XML"
        VERBATIM
)
include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/libxcb-errors/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
add_library(xcb-errors STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/libxcb-errors/src/xcb_errors.c"
        "${CMAKE_CURRENT_BINARY_DIR}/extensions.c"
)

target_link_libraries(xcb-errors xcbproto)

####################################################################################################
########################################## LIBXCVT #################################################
####################################################################################################

add_library(xcvt STATIC "${CMAKE_CURRENT_SOURCE_DIR}/libxcvt/lib/libxcvt.c")
target_include_directories(xcvt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libxcvt/include")

####################################################################################################
####################################### LIBXKB-COMMON ##############################################
####################################################################################################

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/config.h "")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/config.c
  "#include <keymap.h>
  const struct xkb_keymap_format_ops text_v1_keymap_format_ops = {};")
set(XKBCOMMON_SOURCES atom.c context.c context-priv.c keysym.c keysym-utf.c keymap.c keymap-priv.c
                      state.c text.c utf8.c utils.c x11/keymap.c x11/state.c x11/util.c)
list(TRANSFORM XKBCOMMON_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/libxkbcommon/src/)
set(XKBCOMMON_SOURCES ${XKBCOMMON_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/config.c)
add_library(xkbcommon STATIC ${XKBCOMMON_SOURCES})
target_compile_options(xkbcommon PRIVATE
        "-DHAVE_STRNDUP=1" "-DDEFAULT_XKB_LAYOUT=\"us\"" "-DDEFAULT_XKB_MODEL=\"pc105\""
        "-DDEFAULT_XKB_OPTIONS=NULL" "-DDEFAULT_XKB_RULES=\"evdev\"" "-DDEFAULT_XKB_VARIANT=NULL"
        "-DDFLT_XKB_CONFIG_EXTRA_PATH=\"/data/data/com.termux.x11\""
        "-DDFLT_XKB_CONFIG_ROOT=\"/data/data/com.termux.x11\"")
target_include_directories(xkbcommon PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/libxkbcommon/src")
target_include_directories(xkbcommon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libxkbcommon/include")
target_link_libraries(xkbcommon xcbproto)

####################################################################################################
########################################### LORIE ##################################################
####################################################################################################

add_library(lorie SHARED ${CMAKE_CURRENT_SOURCE_DIR}/lorie/lorie-client.cpp)
target_include_directories(lorie PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lorie)
target_link_libraries(lorie xkbcommon xcbproto xcb xcb-errors xcvt "-llog" "-landroid"
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/lorie.version")
