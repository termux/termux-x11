set(SERVER_MISC_CONFIG_PATH "/usr/lib/xorg")
set(PROJECTROOT "/usr")
set(SYSCONFDIR "/usr/etc")
set(SUID_WRAPPER_DIR "/usr/libexec")
set(COMPILEDDEFAULTFONTPATH "/usr/share/fonts/X11/misc,/usr/share/fonts/X11/TTF,/usr/share/fonts/X11/OTF,/usr/share/fonts/X11/Type1,/usr/share/fonts/X11/100dpi,/usr/share/fonts/X11/75dpi")
set(DRI_DRIVER_PATH "\"/usr/lib/x86_64-linux-gnu/dri\"")

check_source_compiles(C "int main() {}; int foo(int bar) { typeof(bar) baz = 1; return baz; }" HAVE_TYPEOF)
check_function_exists("clock_gettime" HAVE_CLOCK_GETTIME)
check_source_compiles(C "
    #define _POSIX_C_SOURCE 200112L
    #include <time.h>
    #include <unistd.h>
    #ifndef CLOCK_MONOTONIC
    #error CLOCK_MONOTONIC not defined
    #endif
    int main() {}
" HAVE_CLOCK_MONOTONIC)
if (HAVE_CLOCK_GETTIME AND HAVE_CLOCK_MONOTONIC)
    set(MONOTONIC_CLOCK 1)
endif()
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    set(X_BYTE_ORDER "X_BIG_ENDIAN")
else()
    set(X_BYTE_ORDER "X_LITTLE_ENDIAN")
endif()
check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)
if (SIZEOF_UNSIGNED_LONG EQUAL 8)
    set(_XSERVER64 1)
endif ()
check_source_compiles(C "
    #define _GNU_SOURCE 1
    #include <pthread.h>
    int main() {PTHREAD_MUTEX_RECURSIVE;}
" HAVE_PTHREAD_MUTEX_RECURSIVE)
if (HAVE_PTHREAD_MUTEX_RECURSIVE)
    set(INPUTTHREAD 1)
endif()

set(CMAKE_REQUIRED_FLAGS_OLD ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_DEFINITIONS_OLD ${CMAKE_REQUIRED_DEFINITIONS})
set(CMAKE_REQUIRED_LIBRARIES_OLD ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_FLAGS "-Werror-implicit-function-declaration" "-lm")
set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
set(CMAKE_REQUIRED_LIBRARIES m)
check_source_compiles(C "
    #define _GNU_SOURCE 1
    #include <pthread.h>
    int main() { pthread_setname_np(pthread_self(), \"example\"); }
" HAVE_PTHREAD_SETNAME_NP_WITH_TID)
check_source_compiles(C "
    #define _GNU_SOURCE 1
    #include <pthread.h>
    int main() { pthread_setname_np(\"example\"); }
" HAVE_PTHREAD_SETNAME_NP_WITHOUT_TID)

check_include_file("bsd/stdlib.h" HAVE_LIBBSD)
check_include_file("dlfcn.h" HAVE_DLFCN_H)
check_include_file("execinfo.h" HAVE_EXECINFO_H)
check_include_file("fnmatch.h" HAVE_FNMATCH_H)
check_include_file("linux/agpgart.h" HAVE_LINUX_AGPGART_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("sys/agpgart.h" HAVE_SYS_AGPGART_H)
check_include_file("sys/un.h" HAVE_SYS_UN_H)
check_include_file("sys/utsname.h" HAVE_SYS_UTSNAME_H)
check_include_file("sys/sysmacros.h" HAVE_SYS_SYSMACROS_H)

check_function_exists("arc4random_buf" HAVE_ARC4RANDOM_BUF)
# Android does not have `execinfo.h`. And in case of static build it is pointless, no symbol names will be available.
check_function_exists("backtrace" HAVE_BACKTRACE)
check_function_exists("cbrt" HAVE_CBRT)
check_function_exists("epoll_create1" HAVE_EPOLL_CREATE1)
check_function_exists("getuid" HAVE_GETUID)
check_function_exists("geteuid" HAVE_GETEUID)
check_function_exists("isastream" HAVE_ISASTREAM)
check_function_exists("issetugid" HAVE_ISSETUGID)
check_function_exists("getifaddrs" HAVE_GETIFADDRS)
check_function_exists("getpeereid" HAVE_GETPEEREID)
check_function_exists("getpeerucred" HAVE_GETPEERUCRED)
check_function_exists("getprogname" HAVE_GETPROGNAME)
check_function_exists("getzoneid" HAVE_GETZONEID)
# We emulate this
# check_function_exists("memfd_create" HAVE_MEMFD_CREATE)
set(HAVE_MEMFD_CREATE 1)
check_function_exists("mkostemp" HAVE_MKOSTEMP)
check_function_exists("mmap" HAVE_MMAP)
check_function_exists("open_device" HAVE_OPEN_DEVICE)
check_function_exists("poll" HAVE_POLL)
check_function_exists("pollset_create" HAVE_POLLSET_CREATE)
check_function_exists("posix_fallocate" HAVE_POSIX_FALLOCATE)
check_function_exists("port_create" HAVE_PORT_CREATE)
check_function_exists("reallocarray" HAVE_REALLOCARRAY)
check_function_exists("seteuid" HAVE_SETEUID)
check_function_exists("setitimer" HAVE_SETITIMER)
# Solaris-specific
# check_function_exists("shmctl64" HAVE_SHMCTL64)
check_function_exists("sigaction" HAVE_SIGACTION)
check_function_exists("sigprocmask" HAVE_SIGPROCMASK)
check_function_exists("strcasecmp" HAVE_STRCASECMP)
check_function_exists("strcasestr" HAVE_STRCASESTR)
check_function_exists("strlcat" HAVE_STRLCAT)
check_function_exists("strlcpy" HAVE_STRLCPY)
check_function_exists("strncasecmp" HAVE_STRNCASECMP)
check_function_exists("strncasecmp" HAVE_STRNDUP)
check_function_exists("timingsafe_memcmp" HAVE_TIMINGSAFE_MEMCMP)
check_function_exists("vasprintf" HAVE_VASPRINTF)
check_function_exists("vsnprintf" HAVE_VSNPRINTF)
check_function_exists("walkcontext" HAVE_WALKCONTEXT)
check_function_exists("shmctl" HAVE_SHMCTL)
set(BUSFAULT ${HAVE_SIGACTION})

check_source_compiles(C "
    #include <sys/socket.h>
    int main() { SCM_RIGHTS; }
" XTRANS_SEND_FDS)
set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_OLD})
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS_OLD})
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_OLD})

configure_file("patches/dix-config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/dix-config.h")

file(GENERATE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version-config.h
        CONTENT "
#pragma once
#define VENDOR_MAN_VERSION \"Version 21.1.99\"
#define VENDOR_NAME \"The X.Org Foundation\"
#define VENDOR_NAME_SHORT \"X.Org\"
#define VENDOR_RELEASE 12101099
#define VENDOR_WEB \"http://wiki.x.org\"
")

file(GENERATE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xkb-config.h
        CONTENT "
#pragma once
// #define XKB_BASE_DIRECTORY \"/dev/null/data/data/com.termux/files/usr/share/X11/xkb\"
#define XKB_BASE_DIRECTORY \"/usr/share/X11/xkb/\"
#define XKB_BIN_DIRECTORY \"\"
#define XKB_DFLT_LAYOUT \"us\"
#define XKB_DFLT_MODEL \"pc105\"
#define XKB_DFLT_OPTIONS \"\"
#define XKB_DFLT_RULES \"evdev\"
#define XKB_DFLT_VARIANT \"\"
#define XKM_OUTPUT_DIR (getenv(\"TMPDIR\") ?: \"/tmp\")
")

set(inc ${CMAKE_CURRENT_BINARY_DIR}
        "mesa/include"
        "libxfont/include"
        "pixman/pixman"
        "xorgproto/include"
        "libxkbfile/include"
        "xserver/Xext"
        "xserver/Xi"
        "xserver/composite"
        "xserver/damageext"
        "xserver/exa"
        "xserver/fb"
        "xserver/glamor"
        "xserver/mi"
        "xserver/miext/damage"
        "xserver/miext/shadow"
        "xserver/miext/sync"
        "xserver/dbe"
        "xserver/dri3"
        "xserver/include"
        "xserver/present"
        "xserver/randr"
        "xserver/render"
        "xserver/xfixes"
        "xserver/glx")

# _FILE_OFFSET_BITS=64 ??
set(compile_options
        ${common_compile_options}
        "-std=gnu99"
        "-DHAVE_DIX_CONFIG_H"
        "-fno-strict-aliasing"
        "-fvisibility=hidden"
        "-fPIC"
        "-D_DEFAULT_SOURCE"
        "-D_BSD_SOURCE"
        "-DHAS_FCHOWN"
        "-DHAS_STICKY_DIR_BIT"
        "-D_PATH_TMP=getenv(\"TMPDIR\")?:\"/tmp\""
        "-include" "${CMAKE_CURRENT_SOURCE_DIR}/lorie/shm/shm.h")

#set(compile_options ${compile_options} "-finstrument-functions")

set(DIX_SOURCES
        atom.c colormap.c cursor.c devices.c dispatch.c dixfonts.c main.c dixutils.c enterleave.c
        events.c eventconvert.c extension.c gc.c gestures.c getevents.c globals.c glyphcurs.c
        grabs.c initatoms.c inpututils.c pixmap.c privates.c property.c ptrveloc.c region.c
        registry.c resource.c selection.c swaprep.c swapreq.c tables.c touch.c window.c)
list(TRANSFORM DIX_SOURCES PREPEND "xserver/dix/")
add_library(xserver_dix STATIC ${DIX_SOURCES})
target_include_directories(xserver_dix PRIVATE ${inc})
target_compile_options(xserver_dix PRIVATE ${compile_options})

add_library(xserver_main STATIC
        "xserver/dix/stubmain.c")
target_include_directories(xserver_main PRIVATE ${inc})
target_compile_options(xserver_main PRIVATE ${compile_options})

file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/drm_fourcc.h"
        CONTENT "
    #pragma once
    #define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
    #define DRM_FORMAT_RGB565	fourcc_code('R', 'G', '1', '6')
    #define DRM_FORMAT_XRGB8888	fourcc_code('X', 'R', '2', '4')
    #define DRM_FORMAT_XRGB2101010	fourcc_code('X', 'R', '3', '0')
    #define DRM_FORMAT_ARGB8888	fourcc_code('A', 'R', '2', '4')
    #define DRM_FORMAT_MOD_INVALID -1
")
add_library(xserver_dri3 STATIC
        "xserver/dri3/dri3.c"
        "xserver/dri3/dri3_request.c"
        "xserver/dri3/dri3_screen.c")
target_include_directories(xserver_dri3 PRIVATE ${inc})
target_compile_options(xserver_dri3 PRIVATE ${compile_options})

set(FB_SOURCES
        fballpriv.c fbarc.c fbbits.c fbblt.c fbbltone.c fbcmap_mi.c fbcopy.c fbfill.c fbfillrect.c
        fbfillsp.c fbgc.c fbgetsp.c fbglyph.c fbimage.c fbline.c fboverlay.c fbpict.c fbpixmap.c
        fbpoint.c fbpush.c fbscreen.c fbseg.c fbsetsp.c fbsolid.c fbtrap.c fbutil.c fbwindow.c)
list(TRANSFORM FB_SOURCES PREPEND "xserver/fb/")
add_library(xserver_fb STATIC ${FB_SOURCES})
target_include_directories(xserver_fb PRIVATE ${inc})
target_compile_options(xserver_fb PRIVATE ${compile_options})

set(MI_SOURCES
        miarc.c mibitblt.c micmap.c micopy.c midash.c midispcur.c mieq.c miexpose.c mifillarc.c
        mifillrct.c migc.c miglblt.c mioverlay.c mipointer.c mipoly.c mipolypnt.c mipolyrect.c
        mipolyseg.c mipolytext.c mipushpxl.c miscrinit.c misprite.c mivaltree.c miwideline.c
        miwindow.c mizerarc.c mizerclip.c mizerline.c)
list(TRANSFORM MI_SOURCES PREPEND "xserver/mi/")
add_library(xserver_mi STATIC ${MI_SOURCES})
target_include_directories(xserver_mi PRIVATE ${inc})
target_compile_options(xserver_mi PRIVATE ${compile_options})

set(OS_SOURCES
        WaitFor.c access.c auth.c backtrace.c client.c connection.c inputthread.c io.c mitauth.c
        oscolor.c osinit.c ospoll.c utils.c xdmauth.c xsha1.c xstrans.c xprintf.c log.c strlcpy.c
        busfault.c timingsafe_memcmp.c rpcauth.c xdmcp.c)
list(TRANSFORM OS_SOURCES PREPEND "xserver/os/")
add_library(xserver_os STATIC ${OS_SOURCES})
# TODO: add android-specific sources to avoid incompatibility...
target_include_directories(xserver_os PRIVATE ${inc})
target_link_libraries(xserver_os PRIVATE md Xtrans Xdmcp Xau tirpc)
target_compile_options(xserver_os PRIVATE ${compile_options} "-DCLIENTIDS")

set(COMPOSITE_SOURCES compalloc.c compext.c compinit.c compoverlay.c compwindow.c)
list(TRANSFORM COMPOSITE_SOURCES PREPEND "xserver/composite/")
add_library(xserver_composite STATIC ${COMPOSITE_SOURCES})
target_include_directories(xserver_composite PRIVATE ${inc})
target_compile_options(xserver_composite PRIVATE ${compile_options})

add_library(xserver_damageext STATIC "xserver/damageext/damageext.c")
target_include_directories(xserver_damageext PRIVATE ${inc})
target_compile_options(xserver_damageext PRIVATE ${compile_options})

add_library(xserver_dbe STATIC "xserver/dbe/dbe.c" "xserver/dbe/midbe.c")
target_include_directories(xserver_dbe PRIVATE ${inc})
target_compile_options(xserver_dbe PRIVATE ${compile_options})

add_library(xserver_miext_damage STATIC "xserver/miext/damage/damage.c")
target_include_directories(xserver_miext_damage PRIVATE ${inc})
target_compile_options(xserver_miext_damage PRIVATE ${compile_options})

set(MIEXT_SHADOW_SOURCES
        shadow.c sh3224.c shafb4.c shafb8.c shiplan2p4.c shiplan2p8.c shpacked.c shplanar8.c
        shplanar.c shrot16pack_180.c shrot16pack_270.c shrot16pack_270YX.c shrot16pack_90.c
        shrot16pack_90YX.c shrot16pack.c shrot32pack_180.c shrot32pack_270.c shrot32pack_90.c
        shrot32pack.c shrot8pack_180.c shrot8pack_270.c shrot8pack_90.c shrot8pack.c shrotate.c)
list(TRANSFORM MIEXT_SHADOW_SOURCES PREPEND "xserver/miext/shadow/")
add_library(xserver_miext_shadow STATIC ${MIEXT_SHADOW_SOURCES})
target_include_directories(xserver_miext_shadow PRIVATE ${inc})
target_compile_options(xserver_miext_shadow PRIVATE ${compile_options})

set(MIEXT_SHADOW_SOURCES misync.c misyncfd.c misyncshm.c)
list(TRANSFORM MIEXT_SHADOW_SOURCES PREPEND "xserver/miext/sync/")
add_library(xserver_miext_sync STATIC ${MIEXT_SHADOW_SOURCES})
target_include_directories(xserver_miext_sync PRIVATE ${inc})
target_compile_options(xserver_miext_sync PRIVATE ${compile_options})

set(MIEXT_ROOTLESS_SOURCES Common.c GC.c Screen.c ValTree.c Window.c)
list(TRANSFORM MIEXT_ROOTLESS_SOURCES PREPEND "xserver/miext/rootless/rootless")
add_library(xserver_miext_rootless STATIC ${MIEXT_ROOTLESS_SOURCES})
target_include_directories(xserver_miext_rootless PRIVATE ${inc})
target_compile_options(xserver_miext_rootless PRIVATE ${compile_options})

set(PRESENT_SOURCES
        present.c present_event.c present_execute.c present_fake.c present_fence.c present_notify.c
        present_request.c present_scmd.c present_screen.c present_vblank.c)
list(TRANSFORM PRESENT_SOURCES PREPEND "xserver/present/")
add_library(xserver_present STATIC ${PRESENT_SOURCES})
target_include_directories(xserver_present PRIVATE ${inc})
target_compile_options(xserver_present PRIVATE ${compile_options})

set(RANDR_SOURCES
        randr.c rrcrtc.c rrdispatch.c rrinfo.c rrlease.c rrmode.c rrmonitor.c rroutput.c rrpointer.c
        rrproperty.c rrprovider.c rrproviderproperty.c rrscreen.c rrsdispatch.c rrtransform.c
        rrxinerama.c)
list(TRANSFORM RANDR_SOURCES PREPEND "xserver/randr/")
add_library(xserver_randr STATIC ${RANDR_SOURCES})
target_include_directories(xserver_randr PRIVATE ${inc})
target_compile_options(xserver_randr PRIVATE ${compile_options})

add_library(xserver_record STATIC xserver/record/record.c xserver/record/set.c)
target_include_directories(xserver_record PRIVATE ${inc})
target_compile_options(xserver_record PRIVATE ${compile_options})

set(RENDER_SOURCES
        animcur.c filter.c glyph.c matrix.c miindex.c mipict.c mirect.c mitrap.c mitri.c picture.c
        render.c)
list(TRANSFORM RENDER_SOURCES PREPEND "xserver/render/")
add_library(xserver_render STATIC ${RENDER_SOURCES})
target_include_directories(xserver_render PRIVATE ${inc})
target_compile_options(xserver_render PRIVATE ${compile_options})

set(XFIXES_SOURCES cursor.c disconnect.c region.c saveset.c select.c xfixes.c)
list(TRANSFORM XFIXES_SOURCES PREPEND "xserver/xfixes/")
add_library(xserver_xfixes STATIC ${XFIXES_SOURCES})
target_include_directories(xserver_xfixes PRIVATE ${inc})
target_compile_options(xserver_xfixes PRIVATE ${compile_options})

set(XKB_SOURCES
        ddxBeep.c ddxCtrls.c ddxLEDs.c ddxLoad.c maprules.c xkmread.c xkbtext.c xkbfmisc.c xkbout.c
        xkb.c xkbUtils.c xkbEvents.c xkbAccessX.c xkbSwap.c xkbLEDs.c xkbInit.c xkbActions.c
        xkbPrKeyEv.c XKBMisc.c XKBAlloc.c XKBGAlloc.c XKBMAlloc.c)
list(TRANSFORM XKB_SOURCES PREPEND "xserver/xkb/")
add_library(xserver_xkb STATIC ${XKB_SOURCES})
target_include_directories(xserver_xkb PRIVATE ${inc})
target_compile_options(xserver_xkb PRIVATE ${compile_options} "-DXkbFreeGeomOverlayKeys=XkbFreeGeomOverlayKeysInternal")

set(XKB_STUBS_SOURCES ddxKillSrv.c ddxPrivate.c ddxVT.c)
list(TRANSFORM XKB_STUBS_SOURCES PREPEND "xserver/xkb/")
add_library(xserver_xkb_stubs STATIC ${XKB_STUBS_SOURCES})
target_include_directories(xserver_xkb_stubs PRIVATE ${inc})
target_compile_options(xserver_xkb_stubs PRIVATE ${compile_options})

set(XEXT_SOURCES
        bigreq.c geext.c shape.c sleepuntil.c sync.c xcmisc.c xtest.c dpms.c shm.c hashtable.c
        xres.c saver.c xace.c xf86bigfont.c panoramiX.c panoramiXprocs.c panoramiXSwap.c xvmain.c
        xvdisp.c xvmc.c vidmode.c)
list(TRANSFORM XEXT_SOURCES PREPEND "xserver/Xext/")
add_library(xserver_xext STATIC ${XEXT_SOURCES})
target_include_directories(xserver_xext PRIVATE ${inc})
target_compile_options(xserver_xext PRIVATE ${compile_options})

set(XI_SOURCES
        allowev.c chgdctl.c chgfctl.c chgkbd.c chgkmap.c chgprop.c chgptr.c closedev.c devbell.c
        exevents.c extinit.c getbmap.c getdctl.c getfctl.c getfocus.c getkmap.c getmmap.c getprop.c
        getselev.c getvers.c grabdev.c grabdevb.c grabdevk.c gtmotion.c listdev.c opendev.c queryst.c
        selectev.c sendexev.c setbmap.c setdval.c setfocus.c setmmap.c setmode.c ungrdev.c ungrdevb.c
        ungrdevk.c xiallowev.c xibarriers.c xichangecursor.c xichangehierarchy.c xigetclientpointer.c
        xigrabdev.c xipassivegrab.c xiproperty.c xiquerydevice.c xiquerypointer.c xiqueryversion.c
        xiselectev.c xisetclientpointer.c xisetdevfocus.c xiwarppointer.c)
list(TRANSFORM XI_SOURCES PREPEND "xserver/Xi/")
add_library(xserver_xi STATIC ${XI_SOURCES})
target_include_directories(xserver_xi PRIVATE ${inc})
target_compile_options(xserver_xi PRIVATE ${compile_options})

add_library(xserver_xi_stubs STATIC "xserver/Xi/stubs.c")
target_include_directories(xserver_xi_stubs PRIVATE ${inc})
target_compile_options(xserver_xi_stubs PRIVATE ${compile_options})

set(GLX_SOURCES
        glxext.c indirect_dispatch.c indirect_dispatch_swap.c indirect_reqsize.c indirect_size_get.c
        indirect_table.c clientinfo.c createcontext.c extension_string.c indirect_util.c
        indirect_program.c indirect_texture_compression.c glxcmds.c glxcmdsswap.c glxext.c
        glxdriswrast.c glxdricommon.c glxscreens.c render2.c render2swap.c renderpix.c renderpixswap.c
        rensize.c single2.c single2swap.c singlepix.c singlepixswap.c singlesize.c swap_interval.c
        xfont.c)
list(TRANSFORM GLX_SOURCES PREPEND "xserver/glx/")
add_library(xserver_glx STATIC ${GLX_SOURCES})
target_include_directories(xserver_glx PRIVATE ${inc})
target_compile_options(xserver_glx PRIVATE ${compile_options})

set(GLXVND_SOURCES
        vndcmds.c vndext.c vndservermapping.c vndservervendor.c)
list(TRANSFORM GLXVND_SOURCES PREPEND "xserver/glx/")
add_library(xserver_glxvnd STATIC ${GLXVND_SOURCES})
target_include_directories(xserver_glxvnd PRIVATE ${inc})
target_compile_options(xserver_glxvnd PRIVATE ${compile_options})

set(XSERVER_LIBS)
foreach (part glx glxvnd fb mi dix composite damageext dbe randr miext_damage render present xext
              miext_sync xfixes xi xkb record xi_stubs xkb_stubs os dri3)
    set(XSERVER_LIBS ${XSERVER_LIBS} xserver_${part})
endforeach ()

#set(XSERVER_LIBS ${XSERVER_LIBS} tirpc xcb-errors xcb xkbcommon Xdmcp Xau pixman Xfont2 Xtrans freetype fontenc GL)
set(XSERVER_LIBS ${XSERVER_LIBS} tirpc xcb Xdmcp Xau pixman Xfont2 Xtrans freetype fontenc GL)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/tx11.c" "${CMAKE_CURRENT_BINARY_DIR}/tx11.h"
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/c_client.py"
            "-c" "libxcb 1.15" "-l" "X Version 11" "-s" "3" "-p" "xcbproto" "${CMAKE_CURRENT_SOURCE_DIR}/lorie/tx11.xml"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/lorie/tx11.xml" "${CMAKE_CURRENT_SOURCE_DIR}/libxcb/src/c_client.py"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating source code from XML (tx11)"
        VERBATIM
)

add_library(exec-helper SHARED lorie/exec-helper.c)
add_library(Xlorie SHARED
        "xserver/mi/miinitext.c"
        "libxcvt/lib/libxcvt.c"
        "lorie/execl_xkbcomp.c"
        "lorie/shm/shmem.c"
        "lorie/android.cpp"
        "lorie/InitOutput.c"
        "lorie/InitInput.c"
        "lorie/renderer.c"
        "lorie/tx11-request.c"
        "${CMAKE_CURRENT_BINARY_DIR}/tx11.c"
        "${CMAKE_CURRENT_BINARY_DIR}/tx11.h")
target_include_directories(Xlorie PRIVATE ${inc} "libxcvt/include")
target_link_options(Xlorie PRIVATE "-Wl,--as-needed" "-Wl,--no-undefined" "-fvisibility=hidden")
target_link_libraries(Xlorie "-Wl,--whole-archive" ${XSERVER_LIBS} "-Wl,--no-whole-archive" android log m z)
target_compile_options(Xlorie PRIVATE ${compile_options})
add_dependencies(Xlorie xkbcomp exec-helper)
target_apply_patch(Xlorie "${CMAKE_CURRENT_SOURCE_DIR}/xserver" "${CMAKE_CURRENT_SOURCE_DIR}/patches/xserver.patch")
