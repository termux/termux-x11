check_include_file("features.h" HAVE_FEATURES_H)
check_function_exists("getrpcbyname" HAVE_GETRPCBYNAME)
check_function_exists("getrpcbynumber" HAVE_GETRPCBYNUMBER)
check_function_exists("setrpcent" HAVE_SETRPCENT)
check_function_exists("endrpcent" HAVE_ENDRPCENT)
check_function_exists("getrpcent" HAVE_GETRPCENT)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tirpc")
file(CONFIGURE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tirpc/config.h
        CONTENT "
#pragma once
#undef AUTHDES_SUPPORT
#define INET6 1
#cmakedefine01 HAVE_FEATURES_H
#cmakedefine01 HAVE_GETRPCBYNAME
#cmakedefine01 HAVE_GETRPCBYNUMBER
#cmakedefine01 HAVE_SETRPCENT
#cmakedefine01 HAVE_ENDRPCENT
#cmakedefine01 HAVE_GETRPCENT
#undef HAVE_GSSAPI_GSSAPI_EXT_H

#ifdef ANDROID
typedef long long quad_t;
typedef unsigned long long u_quad_t;
#define getdtablesize() sysconf(_SC_OPEN_MAX)
#endif
")
#file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/tirpc/config.h" CONTENT "
#    #pragma once
#    typedef quad_t long long;
#    typedef u_quad_t unsigned long long;")

add_library(tirpc STATIC
        "libtirpc/src/auth_none.c"
        "libtirpc/src/auth_unix.c"
        "libtirpc/src/authunix_prot.c"
        "libtirpc/src/binddynport.c"
        "libtirpc/src/bindresvport.c"
        "libtirpc/src/clnt_bcast.c"
        "libtirpc/src/clnt_dg.c"
        "libtirpc/src/clnt_generic.c"
        "libtirpc/src/clnt_perror.c"
        "libtirpc/src/clnt_raw.c"
        "libtirpc/src/clnt_simple.c"
        "libtirpc/src/clnt_vc.c"
        "libtirpc/src/rpc_dtablesize.c"
        "libtirpc/src/getnetconfig.c"
        "libtirpc/src/getnetpath.c"
        "libtirpc/src/getrpcent.c"
        "libtirpc/src/getrpcport.c"
        "libtirpc/src/mt_misc.c"
        "libtirpc/src/pmap_clnt.c"
        "libtirpc/src/pmap_getmaps.c"
        "libtirpc/src/pmap_getport.c"
        "libtirpc/src/pmap_prot.c"
        "libtirpc/src/pmap_prot2.c"
        "libtirpc/src/pmap_rmt.c"
        "libtirpc/src/rpc_prot.c"
        "libtirpc/src/rpc_commondata.c"
        "libtirpc/src/rpc_callmsg.c"
        "libtirpc/src/rpc_generic.c"
        "libtirpc/src/rpc_soc.c"
        "libtirpc/src/rpcb_clnt.c"
        "libtirpc/src/rpcb_prot.c"
        "libtirpc/src/rpcb_st_xdr.c"
        "libtirpc/src/svc.c"
        "libtirpc/src/svc_auth.c"
        "libtirpc/src/svc_dg.c"
        "libtirpc/src/svc_auth_unix.c"
        "libtirpc/src/svc_auth_none.c"
        "libtirpc/src/svc_generic.c"
        "libtirpc/src/svc_raw.c"
        "libtirpc/src/svc_run.c"
        "libtirpc/src/svc_simple.c"
        "libtirpc/src/svc_vc.c"
        "libtirpc/src/getpeereid.c"
        "libtirpc/src/auth_time.c"
        "libtirpc/src/debug.c"
        "libtirpc/src/xdr.c"
        "libtirpc/src/xdr_rec.c"
        "libtirpc/src/xdr_array.c"
        "libtirpc/src/xdr_float.c"
        "libtirpc/src/xdr_mem.c"
        "libtirpc/src/xdr_reference.c"
        "libtirpc/src/xdr_stdio.c"
        "libtirpc/src/xdr_sizeof.c")
target_include_directories(tirpc PUBLIC "libtirpc/tirpc")
target_compile_options(tirpc PRIVATE "-include" "${CMAKE_CURRENT_BINARY_DIR}/tirpc/config.h" "-DPORTMAP" "-DINET6" "-D_GNU_SOURCE" "-Wall" "-pipe" "-fPIC" "-DPIC")