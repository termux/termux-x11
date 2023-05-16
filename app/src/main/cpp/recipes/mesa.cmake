set(MESA_STUBMAIN "int main(int, char**){return 0;};")
check_source_compiles(C "#ifndef __i386__\n#error\n#endif\n${MESA_STUBMAIN}" HAVE_X86)
check_source_compiles(C "#ifndef __x86_64__\n#error\n#endif\n${MESA_STUBMAIN}" HAVE_X86_64)
check_source_compiles(C "#ifndef __arm__\n#error\n#endif\n${MESA_STUBMAIN}" HAVE_ARM)
check_source_compiles(C "#ifndef __aarch64__\n#error\n#endif\n${MESA_STUBMAIN}" HAVE_ARM64)

set(MESA_BINARIES_DIR "/usr/lib")
if (HAVE_X86)
    set(MESA_CROSS_CPU_FAMILY "x86")
    set(MESA_CROSS_CPU_PREFIX "i686")
    set(MESA_CROSS_CPU "i686")
    if (ANDROID_PLATFORM)
        set(MESA_BINARIES_DIR "${CMAKE_SYSROOT}/usr/lib/i686-linux-android/${CMAKE_SYSTEM_VERSION}/")
    endif ()
endif ()
if (HAVE_X86_64)
    set(MESA_CROSS_CPU_FAMILY "x86_64")
    set(MESA_CROSS_CPU_PREFIX "x86_64")
    set(MESA_CROSS_CPU "x86_64")
    if (ANDROID_PLATFORM)
        set(MESA_BINARIES_DIR "${CMAKE_SYSROOT}/usr/lib/x86_64-linux-android/${CMAKE_SYSTEM_VERSION}/")
    endif ()
endif ()
if (HAVE_ARM)
    set(MESA_CROSS_CPU_FAMILY "arm")
    set(MESA_CROSS_CPU_PREFIX "arm")
    set(MESA_CROSS_CPU "armv7")
    if (ANDROID_PLATFORM)
        set(MESA_BINARIES_DIR "${CMAKE_SYSROOT}/usr/lib/arm-linux-androideabi/${CMAKE_SYSTEM_VERSION}/")
    endif ()
endif ()
if (HAVE_ARM64)
    set(MESA_CROSS_CPU_FAMILY "aarch64")
    set(MESA_CROSS_CPU_PREFIX "aarch64")
    set(MESA_CROSS_CPU "aarch64")
    if (ANDROID_PLATFORM)
        set(MESA_BINARIES_DIR "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-android/${CMAKE_SYSTEM_VERSION}/")
    endif ()
endif ()

cmake_path(GET CMAKE_C_COMPILER PARENT_PATH MESA_TOOLCHAIN_PATH)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/mesa")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/mesa/build")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/mesa/pkgconfig")
set(MESA_COMPILER_ARGS "'--sysroot=${CMAKE_SYSROOT}', '--target=${ANDROID_LLVM_TRIPLE}', '-include', '${CMAKE_CURRENT_SOURCE_DIR}/lorie/shm/shm.h', '-B${MESA_BINARIES_DIR}'")

file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mesa/meson-crossfile"
        CONTENT "[binaries]
pkgconfig = ['env', 'PKG_CONFIG_LIBDIR=${CMAKE_CURRENT_BINARY_DIR}/mesa/pkgconfig', '/usr/bin/pkg-config']
cmake = '${CMAKE_COMMAND}'
c = ['${CMAKE_C_COMPILER}', ${MESA_COMPILER_ARGS}]
cpp =[ '${CMAKE_CXX_COMPILER}', ${MESA_COMPILER_ARGS}]
ar = '${CMAKE_AR}'
ld = '${CMAKE_LINKER}'
strip = '${MESA_TOOLCHAIN_PATH}/llvm-strip'

[properties]
needs_exe_wrapper = true

[host_machine]
cpu_family = '${MESA_CROSS_CPU_FAMILY}'
cpu = '${MESA_CROSS_CPU}'
endian = 'little'
system = 'android'

[built-in options]
cpp_link_args = ['-L${CMAKE_CURRENT_BINARY_DIR}', '-L${MESA_BINARIES_DIR}', '-landroid-shmem', '-nostdlib++', '-lc++_shared']
c_link_args = ['-L${MESA_BINARIES_DIR}']
cpp_args = '${CMAKE_CXX_FLAGS}'
")

find_program(MESON "meson")

function(generate_mesa_pkgconfig libname libversion cflags linkflags)
    file(GENERATE
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mesa/pkgconfig/${libname}.pc"
            CONTENT "\
Name: ${libname}
Description:
Version: ${libversion}
Cflags: ${cflags}
Libs: ${linkflags}
")
endfunction()

add_custom_target(mesa-pkgconfig)
add_custom_target(mesa-generated DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/mesa/build/src/glx/libGL.so" "${CMAKE_CURRENT_BINARY_DIR}/mesa/build/src/mapi/shared-glapi/libglapi.so")

add_dependencies(mesa-generated mesa-pkgconfig)
add_dependencies(mesa-generated drm xcb X11 X11-xcb Xext Xxf86vm Xfixes Xau xshmfence android-shmem)

generate_mesa_pkgconfig("dl" "1.0" "" "-ldl")
generate_mesa_pkgconfig("libdrm" "2.4.113" "-I${CMAKE_CURRENT_SOURCE_DIR}/drm/include/drm -I${CMAKE_CURRENT_SOURCE_DIR}/drm" "-L${CMAKE_CURRENT_BINARY_DIR} -ldrm")
generate_mesa_pkgconfig("x11" "1.8.1" "-I${CMAKE_CURRENT_SOURCE_DIR}/xorgproto/include -I${CMAKE_CURRENT_SOURCE_DIR}/libx11/include -I${CMAKE_CURRENT_SOURCE_DIR}/libxext/include -I${CMAKE_CURRENT_SOURCE_DIR}/libxrandr/include -I${CMAKE_CURRENT_SOURCE_DIR}/libxrender/include -I${CMAKE_CURRENT_SOURCE_DIR}/libxxf86vm/include -I${CMAKE_CURRENT_SOURCE_DIR}/libxfixes/include" "-L${CMAKE_CURRENT_BINARY_DIR} -lX11 -lXtrans")
generate_mesa_pkgconfig("x11-xcb" "1.8.1" "-I${CMAKE_CURRENT_SOURCE_DIR}/xcb -I${CMAKE_CURRENT_SOURCE_DIR}/libx11/include -I${CMAKE_CURRENT_SOURCE_DIR}/libxrender/include" "-L${CMAKE_CURRENT_BINARY_DIR} -lX11-xcb")
generate_mesa_pkgconfig("xext" "1.3.4" "-I${CMAKE_CURRENT_SOURCE_DIR}/libxext/include" "-L${CMAKE_CURRENT_BINARY_DIR} -lXext")
generate_mesa_pkgconfig("xfixes" "6.0.0" "" "-L${CMAKE_CURRENT_BINARY_DIR} -lXfixes")
generate_mesa_pkgconfig("xshmfence" "1.3" "-I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}/libx11/include" "-L${CMAKE_CURRENT_BINARY_DIR} -lxshmfence")
generate_mesa_pkgconfig("glproto" "1.4.17" "" "")
generate_mesa_pkgconfig("dri2proto" "2.8" "" "")
generate_mesa_pkgconfig("xxf86vm" "1.1.4" "" "-L${CMAKE_CURRENT_BINARY_DIR} -lXxf86vm")
generate_mesa_pkgconfig("xrandr" "1.5.2" "" "")
foreach(lib xcb xcb-glx xcb-shm xcb-dri2 xcb-dri3 xcb-present xcb-sync xcb-xfixes xcb-randr)
    generate_mesa_pkgconfig("${lib}" "1.15" "-I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_BINARY_DIR}/xcb -I${CMAKE_CURRENT_SOURCE_DIR}/xorgproto/include -I${CMAKE_CURRENT_SOURCE_DIR}/libxrandr/include" "-L${CMAKE_CURRENT_BINARY_DIR} -lxcb -lXau")
endforeach ()

add_custom_command(
        OUTPUT
            "${CMAKE_CURRENT_BINARY_DIR}/mesa/build/src/glx/libGL.so"
            "${CMAKE_CURRENT_BINARY_DIR}/mesa/build/src/mapi/shared-glapi/libglapi.so"
            "${CMAKE_CURRENT_BINARY_DIR}/mesa/build/src/gallium/targets/dri/swrast_dri.so"
        COMMAND "rm" "-rf" "${CMAKE_CURRENT_BINARY_DIR}/mesa/build" "&&"
            "mkdir" "-p" "${CMAKE_CURRENT_BINARY_DIR}/mesa/build" "&&"
            "cd" "${CMAKE_CURRENT_BINARY_DIR}/mesa/build" "&&"
            "${MESON}" "${CMAKE_CURRENT_SOURCE_DIR}/mesa" "--cross-file=${CMAKE_CURRENT_BINARY_DIR}/mesa/meson-crossfile"
            "-Dcpp_rtti=false" "-Dgbm=disabled" "-Dopengl=true" "-Degl=disabled" "-Degl-native-platform=x11" "-Dgles1=disabled"
            "-Dgles2=disabled" "-Ddri3=enabled" "-Dglx=dri" "-Dllvm=disabled" "-Dshared-llvm=disabled" "-Dplatforms=x11"
            "-Dgallium-drivers=swrast,virgl" "-Dvulkan-drivers=[]" "-Dosmesa=false" "-Dglvnd=false" "-Dxmlconfig=disabled" "&&"
            "ninja" "-j8"
        COMMENT "Building mesa"
        VERBATIM
)

add_library(mesa-swrast STATIC IMPORTED)
set_target_properties(mesa-swrast PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/mesa/build/src/gallium/targets/dri/swrast_dri.so")

add_library(mesa-glapi STATIC IMPORTED)
set_target_properties(mesa-glapi PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/mesa/build/src/mapi/shared-glapi/libglapi.so")

add_library(GL STATIC IMPORTED)
set_target_properties(GL PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/mesa/build/src/glx/libGL.so")
target_link_libraries(GL INTERFACE mesa-glapi mesa-swrast c++_shared)
target_link_options(GL INTERFACE "-L${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(GL INTERFACE "mesa/include")
add_dependencies(GL mesa-generated)
