cmake_minimum_required(VERSION 3.22)
project(lorie)
set(CMAKE_CXX_STANDARD 20)

find_package(Python3 REQUIRED)

####################################################################################################
######################################### LIBWAYLAND ###############################################
####################################################################################################

set(WAYLAND_SOURCES connection.c event-loop.c wayland-protocol.c wayland-server.c wayland-shm.c wayland-os.c wayland-util.c)
list(TRANSFORM WAYLAND_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/wayland/src/)
add_library(wayland-server SHARED ${WAYLAND_SOURCES})
target_include_directories(wayland-server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/wayland ${CMAKE_CURRENT_SOURCE_DIR}/wayland/src)

####################################################################################################
########################################### LORIE ##################################################
####################################################################################################

file(GLOB WAYLAND_GENERATED "${WAYLAND_GENERATED_DIR}/*.cpp")
add_library(lorie SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/lorie/compositor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lorie/lorie_wayland_server.cpp
        ${WAYLAND_GENERATED}
        )
target_include_directories(lorie PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lorie ${WAYLAND_GENERATED_DIR})
target_link_libraries(lorie wayland-server "-landroid" "-llog")

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
add_library(Xau SHARED ${XAU_SOURCES})
target_include_directories(Xau PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libXau/include)
target_link_libraries(Xau xorgproto xcbproto)

####################################################################################################
########################################### LIBXCB #################################################
####################################################################################################

set(XCB_SOURCES xcb_auth.c xcb_conn.c xcb_ext.c xcb_in.c xcb_list.c
        xcb_out.c xcb_util.c xcb_xid.c xcb.h xcbext.h xcbint.h xcb_windefs.h)
list(TRANSFORM XCB_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/)
set(XCB_SOURCES ${XCB_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/loading_sign.c)
add_library(xcb SHARED ${XCB_SOURCES} ${XCB_GENERATED})
target_include_directories(xcb PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/" "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_options(xcb PRIVATE "-DXCB_QUEUE_BUFFER_SIZE=16384" "-DHAVE_SENDMSG")
target_link_libraries(xcb Xau)

# Those are stubs and needed to check if termux-built xcb behaves the same way
# All the functions they should contain are contained in libxcb itself
add_library(xcb-shm SHARED ${CMAKE_CURRENT_SOURCE_DIR}/loading_sign.c)
add_library(xcb-xfixes SHARED ${CMAKE_CURRENT_SOURCE_DIR}/loading_sign.c)
add_library(xcb-xinput SHARED ${CMAKE_CURRENT_SOURCE_DIR}/loading_sign.c)
add_library(xcb-damage SHARED ${CMAKE_CURRENT_SOURCE_DIR}/loading_sign.c)

# TODO: strip unneded symbols to reduce shared library size...

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
add_library(xcb-errors
        "${CMAKE_CURRENT_SOURCE_DIR}/libxcb-errors/src/xcb_errors.c"
        "${CMAKE_CURRENT_BINARY_DIR}/extensions.c"
)

target_link_libraries(xcb-errors xcbproto)

####################################################################################################
########################################## LIBXCVT #################################################
####################################################################################################

add_library(xcvt "${CMAKE_CURRENT_SOURCE_DIR}/libxcvt/lib/libxcvt.c")
target_include_directories(xcvt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libxcvt/include")

####################################################################################################
####################################### LORIE-CLIENT ###############################################
####################################################################################################

add_library(lorie-client SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/lorie-client/lorie-client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lorie-client/lorie_message_queue.cpp)
target_link_libraries(lorie-client xcbproto xcb xcb-errors xcb-shm xcb-xfixes xcb-xinput xcb-damage xcvt "-llog" "-landroid")