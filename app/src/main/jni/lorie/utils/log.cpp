#pragma ide diagnostic ignored "readability-inconsistent-declaration-parameter-name"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#include <dlfcn.h>     // for dladdr
#include <cxxabi.h>    // for __cxa_demangle
#include <cstdarg>

#include <string>
#include <iostream>

#define LOG(prio, ...) { \
    __android_log_print(prio, "LorieProfile", __VA_ARGS__); \
    char p = ((prio == ANDROID_LOG_VERBOSE)?'V':            \
             ((prio == ANDROID_LOG_DEBUG)  ?'D':            \
             ((prio == ANDROID_LOG_INFO)   ?'I':            \
             ((prio == ANDROID_LOG_WARN)   ?'W':            \
             ((prio == ANDROID_LOG_ERROR)   ?'E':           \
             ((prio == ANDROID_LOG_FATAL)   ?'F':'U')))))); \
    printf("%s/%c: ", "LorieProfile", p);                   \
    printf(__VA_ARGS__);                                    \
    printf("\n");                                           \
}

#include "../include/log.h"
#include <pthread.h>
#include <unistd.h>

bool enabled = true;
#define no_instrument void __attribute__((no_instrument_function)) __attribute__ ((visibility ("default")))

using namespace std;
extern "C" {

extern void *blacklist[];
#define skip_blacklisted(f) for (int z=0; blacklist[z]!=NULL; z++) if (blacklist[z]==(f)) return

static thread_local int level = -1;

no_instrument __attribute__((__constructor__(5))) i() {
    if (getenv("LORIE_DEBUG"))
        enabled = true;
    if (getenv("LORIE_NDEBUG"))
        enabled = false;
}

no_instrument print_func(void *func, int enter) {
    Dl_info info;
    if (dladdr(func, &info) && info.dli_sname != nullptr) {
        int status;
        char *demangled = abi::__cxa_demangle(info.dli_sname,nullptr, nullptr, &status);
        LOGD("%d%*c%s %s", gettid(), level, ' ', enter ? ">" : "<", status == 0 ? demangled : info.dli_sname);
        free(demangled);
    }
}

no_instrument __cyg_profile_func_enter (void *func, [[maybe_unused]] void *caller) {
    if (!enabled) return;
	skip_blacklisted(func);
	level++;
	print_func(func, 1);
}

no_instrument __cyg_profile_func_exit (void *func, [[maybe_unused]] void *caller) {
    if (!enabled) return;
	skip_blacklisted(func);
	print_func(func, 0);
	level--;
}

void *blacklist[] = {
        (void*) __cyg_profile_func_enter,
        (void*) __cyg_profile_func_exit,
        (void*) print_func
};
} // extern "C"
