cmake_minimum_required(VERSION 3.22)
project(lorie)
set(CMAKE_CXX_STANDARD 20)
set(can_use_assembler TRUE)
enable_language(ASM)

find_package(Python3 REQUIRED)

####################################################################################################
########################################### LIBFFI #################################################
####################################################################################################

# libffi is cloned from https://android.googlesource.com/platform/external/libffi/, not from upstream repo

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/ffi_gen.h"
        COMMAND "/bin/bash" "${CMAKE_CURRENT_SOURCE_DIR}/libffi/gen_ffi_header.sh"
            "<" "${CMAKE_CURRENT_SOURCE_DIR}/libffi/include/ffi.h.in"
            ">" "${CMAKE_CURRENT_BINARY_DIR}/generated/ffi_gen.h"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libffi/include/ffi.h.in"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated"
        COMMENT "Generating ffi header"
        VERBATIM
)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/generated/ffi_gen.h" PROPERTIES GENERATED TRUE)
set(FFI_SOURCES
        closures.c
        debug.c
        prep_cif.c
        raw_api.c
        types.c)

if (${CMAKE_ANDROID_ARCH_ABI} STREQUAL "armeabi-v7a")
    set(FFI_SOURCES ${FFI_SOURCES}
            arm/sysv.S
            arm/ffi.c)
    set(FFI_EXPORT_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/libffi/linux-arm)
elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "arm64-v8a")
    set(FFI_SOURCES ${FFI_SOURCES}
            aarch64/sysv.S
            aarch64/ffi.c)
    set(FFI_EXPORT_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/libffi/linux-arm64)
elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "x86")
    set(FFI_SOURCES ${FFI_SOURCES}
            x86/sysv.S
            x86/ffi.c)
    set(FFI_EXPORT_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/libffi/linux-x86)
elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "x86_64")
    set(FFI_SOURCES ${FFI_SOURCES}
            x86/unix64.S
            x86/win64.S
            x86/ffi64.c
            x86/ffiw64.c)
    set(FFI_EXPORT_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/libffi/linux-x86_64)
else()
    message(SEND_ERROR "ABI ${CMAKE_ANDROID_ARCH_ABI} is not supported")
endif()

list(TRANSFORM FFI_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/libffi/src/)
add_library(ffi SHARED ${FFI_SOURCES} "${CMAKE_CURRENT_BINARY_DIR}/generated/ffi_gen.h")
target_include_directories(ffi PUBLIC ${FFI_EXPORT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/libffi/include)
target_compile_options(ffi PRIVATE "-DHAVE_AS_X86_PCREL" "-DHAVE_AS_ASCII_PSEUDO_OP")

####################################################################################################
######################################### LIBWAYLAND ###############################################
####################################################################################################

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wayland/config.h"
        COMMAND "touch" "${CMAKE_CURRENT_BINARY_DIR}/wayland/config.h"
        COMMENT "Generating wayland config header"
        VERBATIM
)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-version.h"
        COMMAND "touch" "${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-version.h"
        COMMENT "Generating wayland version header"
        VERBATIM
)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-server-protocol.c"
        COMMAND "wayland-scanner" "private-code"
        "${CMAKE_CURRENT_SOURCE_DIR}/wayland/protocol/wayland.xml"
        "${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-server-protocol.c"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wayland/protocol/wayland.xml"
        COMMENT "Generating wayland core protocol source"
        VERBATIM
)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-server-protocol.h"
        COMMAND "wayland-scanner" "server-header"
        "${CMAKE_CURRENT_SOURCE_DIR}/wayland/protocol/wayland.xml"
        "${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-server-protocol.h"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wayland/protocol/wayland.xml"
        COMMENT "Generating wayland core protocol header"
        VERBATIM
)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wayland/xdg-shell-protocol.c"
        COMMAND "wayland-scanner" "private-code"
            "${CMAKE_CURRENT_SOURCE_DIR}/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
            "${CMAKE_CURRENT_BINARY_DIR}/wayland/xdg-shell-protocol.c"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
        COMMENT "Generating xdg shell protocol source"
        VERBATIM
)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wayland/xdg-shell-protocol.h"
        COMMAND "wayland-scanner" "server-header"
            "${CMAKE_CURRENT_SOURCE_DIR}/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
            "${CMAKE_CURRENT_BINARY_DIR}/wayland/xdg-shell-protocol.h"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
        COMMENT "Generating xdg shell protocol header"
        VERBATIM
)

set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/wayland/config.h" PROPERTIES GENERATED TRUE)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-version.h" PROPERTIES GENERATED TRUE)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-server-protocol.c" PROPERTIES GENERATED TRUE)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-server-protocol.h" PROPERTIES GENERATED TRUE)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/wayland/xdg-shell-protocol.c" PROPERTIES GENERATED TRUE)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/wayland/xdg-shell-protocol.h" PROPERTIES GENERATED TRUE)

set(WAYLAND_SOURCES connection.c event-loop.c wayland-server.c wayland-shm.c wayland-os.c wayland-util.c)
list(TRANSFORM WAYLAND_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/wayland/src/)
add_library(wayland-server STATIC
        ${WAYLAND_SOURCES}
        "${CMAKE_CURRENT_BINARY_DIR}/wayland/config.h"
        "${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-version.h"
        "${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-server-protocol.c"
        "${CMAKE_CURRENT_BINARY_DIR}/wayland/wayland-server-protocol.h"
        "${CMAKE_CURRENT_BINARY_DIR}/wayland/xdg-shell-protocol.c"
        "${CMAKE_CURRENT_BINARY_DIR}/wayland/xdg-shell-protocol.h")
target_link_libraries(wayland-server ffi)
target_compile_options(wayland-server PUBLIC
        "-DWAYLAND_VERSION_MAJOR=1"
        "-DWAYLAND_VERSION_MINOR=21"
        "-DWAYLAND_VERSION_MICRO=0"
        "-DWAYLAND_VERSION=\"1.21.0\"")
target_include_directories(wayland-server PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/wayland
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/src)

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

file(GLOB WAYLAND_GENERATED "${WAYLAND_GENERATED_DIR}/*.cpp")
add_library(lorie SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/lorie/compositor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lorie/lorie-client.cpp)
target_include_directories(lorie PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lorie)
target_link_libraries(lorie wayland-server xkbcommon xcbproto xcb xcb-errors xcvt "-llog" "-landroid"
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/lorie.version")
