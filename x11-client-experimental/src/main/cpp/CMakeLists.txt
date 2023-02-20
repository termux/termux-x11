cmake_minimum_required(VERSION 3.22)
project(libxcb)

find_package(Python3 REQUIRED)

# Nothing special here, only generating xcb headers and building xcb...

####################################################################################################
######################################### XCB PROTOS ###############################################
####################################################################################################

set (XCB_GENERATED)
set(codegen "${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/c_client.py")
set(site-packages "${CMAKE_CURRENT_SOURCE_DIR}/xcbproto")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(GLOB xcbprotos "${CMAKE_CURRENT_SOURCE_DIR}/xcbproto/src/*.xml")

foreach (xcbproto IN LISTS xcbprotos)
    get_filename_component(name "${xcbproto}" NAME)
    string(REGEX REPLACE "\\.xml$" ".c" gensrc "${CMAKE_CURRENT_BINARY_DIR}/generated/${name}")
    string(REGEX REPLACE "\\.xml$" ".h" genhdr "${CMAKE_CURRENT_BINARY_DIR}/generated/${name}")
    add_custom_command(
            OUTPUT "${gensrc}" "${genhdr}"
            COMMAND Python3::Interpreter "${codegen}" "-c" "libxcb 1.15"
                                "-l" "X Version 11" "-s" "3" "-p" "${site-packages}" "${xcbproto}"
            DEPENDS "${header}" "${codegen_py}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated"
            COMMENT "Generating source code from XML (${xcbproto})"
            VERBATIM
    )
    set (XCB_GENERATED ${XCB_GENERATED} ${gensrc} ${genhdr})

endforeach ()

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
add_library(libXau SHARED ${XAU_SOURCES})
target_include_directories(libXau PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libXau/include)
target_link_libraries(libXau xorgproto)

####################################################################################################
########################################### LIBXCB #################################################
####################################################################################################

set(XCB_SOURCES xcb_auth.c xcb_conn.c xcb_ext.c xcb_in.c xcb_list.c
        xcb_out.c xcb_util.c xcb_xid.c xcb.h xcbext.h xcbint.h xcb_windefs.h)
list(TRANSFORM XCB_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/)
include_directories("libxcb/src" "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_library(libxcb SHARED ${XCB_SOURCES} ${XCB_GENERATED})
target_compile_options(libxcb PRIVATE "-DXCB_QUEUE_BUFFER_SIZE=16384")
target_link_libraries(libxcb libXau)