#include <dlfcn.h>     // for dladdr
#include <cxxabi.h>    // for __cxa_demangle
#include <cstdarg>

#include <string>
#include <iostream>

#include <log.h>
#include <pthread.h>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

static int enabled = 0;

using namespace std;
extern "C" {

extern void *blacklist[];
#define skip_blacklisted(f) for (int z=0; blacklist[z]!=NULL; z++) if (blacklist[z]==f) return;

static bool lock_needed = 
#ifndef __ANDROID__
	true;
#else
	false;
#endif

static pthread_mutex_t lock;

// see https://github.com/littlstar/asprintf.c/blob/master/asprintf.c
static int
_vasprintf_ (char **str, const char *fmt, va_list args) {
	char *dest = NULL;
	int size = 0;
	va_list tmpa;

	va_copy(tmpa, args);
	size = vsnprintf(dest, size, fmt, tmpa);
	va_end(tmpa);

	if (size < 0) { return -1; }
	*str = (char *) malloc(size + 1);

	if (NULL == *str) { return -1; }

	size = vsprintf(*str, fmt, args);
	return size;
}

static int
_asprintf_ (char **str, const char *fmt, ...) {
	int size = 0;
	va_list args;

	va_start(args, fmt);
	size = _vasprintf_(str, fmt, args);
	va_end(args);

	return size;
}

void defaultLogFn(int priority, const char *format, va_list ap) {
	char *buffer;
	char *prio = NULL;
	switch (priority) {
		case LOG_VERBOSE: prio = (char*)"V: "; break;
		case LOG_DEBUG: prio = (char*)"D: "; break;
		case LOG_INFO: prio = (char*)"I: "; break;
		case LOG_WARN: prio = (char*)"W: "; break;
		case LOG_ERROR: prio = (char*)"E: "; break;
		case LOG_FATAL: prio = (char*)"F: "; break;
		case LOG_PROFILER: prio = (char*)"P: "; break;
		default: prio = (char*)"D: ";
	}
	_asprintf_(&buffer, "%s%s\n", prio, format);
	vprintf(buffer, ap);
	free(buffer);
	if (priority == LOG_FATAL) exit(1);
}

#if defined(__ANDROID__)
void androidLogFn(int prio, const char *format, va_list ap) {
	__android_log_vprint(prio, "LorieNative", format, ap);
	if (prio == LOG_FATAL) exit(1);
}
#endif

static void (*logFunction)(int, const char*, va_list) = 
#if !defined(__ANDROID__)
	defaultLogFn;
#else
	androidLogFn;
#endif

static void LogMessageInternal(int priority, const char *format, ...) {
	if (logFunction == NULL) return;

	va_list argptr;
	va_start(argptr, format);
	logFunction(priority, format, argptr);
	va_end(argptr);
}

void LogMessage(int priority, const char *format, ...) {
	if (logFunction == NULL) return;

    if (lock_needed) pthread_mutex_lock(&::lock);
	va_list argptr;
	va_start(argptr, format);
	logFunction(priority, format, argptr);
	va_end(argptr);
    if (lock_needed) pthread_mutex_unlock(&::lock);
}

void LogInit(void) {
	if (lock_needed && pthread_mutex_init(&::lock, NULL) != 0) {
		LogMessageInternal(LOG_ERROR, "Logger mutex init failed\n");
		return;
	}

	LOGD("Logging initialized");
}

static thread_local int level = -1;

void print_func(void *func, int enter);

void __attribute__((no_instrument_function)) __cyg_profile_func_enter (void *this_fn, void *call_site);
void __attribute__((no_instrument_function)) __cyg_profile_func_exit (void *this_fn, void *call_site);

void __attribute__((no_instrument_function))
__cyg_profile_func_enter (void *func,  void *caller) {
	if (!enabled) return;
	skip_blacklisted(func);
	level++;
	print_func(func, 1);
}
 
void __attribute__((no_instrument_function))
__cyg_profile_func_exit (void *func, void *caller) {
	if (!enabled) return;
	skip_blacklisted(func);
	print_func(func, 0);
	level--;
}

void print_func(void *func, int enter) {
	Dl_info info;
	if (dladdr(func, &info) && info.dli_sname != NULL) {
		char *demangled = NULL;
		int status;
		demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
		LOGP("%*c%s %s", level, ' ', enter ? ">" : "<", status == 0 ? demangled : info.dli_sname);
		free(demangled);
	}
}

void *blacklist[] = {
	(void*) _vasprintf_,
	(void*) _asprintf_,
	(void*) defaultLogFn,
#if defined(__ANDROID__)
	(void*) androidLogFn,
#endif
	(void*) LogInit,
	(void*) LogMessage, 
	(void*) LogMessageInternal, 
	(void*) __cyg_profile_func_enter,
	(void*) __cyg_profile_func_exit,
	(void*) print_func
};

} // extern "C"
