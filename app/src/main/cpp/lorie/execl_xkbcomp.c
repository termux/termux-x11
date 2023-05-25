#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/signal.h>
#include <android/log.h>
#include <libgen.h>
#include "whereami.c"

#define maybe_unused __attribute__((unused))

#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "UnusedValue"
#pragma ide diagnostic ignored "ConstantParameter"

int execl_xkbcomp(const char * path, const char * arg, ...);

static char** parse_arguments(const char* line, int* argc) {
    char* cmdline = strdup(line);

    char** argv = (char**)malloc(strlen(cmdline));
    int arg_index = 0;

    int in_quotes = 0;
    long token_len;
    const char* token_start = cmdline;
    for(char* p = cmdline;;p++) {
        if ((*p == ' ' && !in_quotes) || *p == '\0') {
            if (*token_start == '"')
                token_start++;
            token_len = p - token_start;
            if (token_start[token_len-1] == '"')
                token_len--;

            if (token_len > 0) {
                argv[arg_index] = (char *) malloc(sizeof(char) * (token_len + 1));
                memcpy(argv[arg_index], token_start, token_len);
                argv[arg_index][token_len] = '\0';
                arg_index++;
                token_start = p + 1;
            }

            if (*p == '\0') {
                argv[arg_index] = NULL;
                break;
            }
        } else if (*p == '"' && *(p - 1) != '\\') {
            in_quotes = !in_quotes;
        }
    }

    if (argc)
        *argc = arg_index;

    free((void*) cmdline);
    return argv;
}

void LogMessageVerb(int type, int verb, const char *format, ...);

int execl_xkbcomp(const char * path, const char * arg, ...) {
    size_t argv_max = 1024;
    maybe_unused int length, argc;
    maybe_unused char** new_args;
    const char **argv = alloca(argv_max * sizeof(const char *));
    unsigned int i;

    // It is not possible to use exec* functions with binaries inside apk.
    // But it is possible to dlopen libraries.
    // So I will use /system/bin/app_process with overloaded __libc_init to load main function stored in packed binary.
    // We are loading 32-bit libraries so it can fail on 64-bit system.
    // That is why we should execute app_process32 and fall back to sh in the case of failure.
    char lib[1024], ldpreload[1024], ldlibexec[1024];

    va_list args;
    va_start(args, arg);

    argv[0] = arg;

    i = 0;
    while (argv[i++] != NULL) {
        if (i == argv_max) {
            const char **nptr = alloca((argv_max *= 2) * sizeof(const char *));

            if ((char *) argv + i == (char *) nptr)
                argv_max += i;
            else
                argv = (const char **) memcpy(nptr, argv, i * sizeof(const char *));
        }

        argv[i] = va_arg(args, const char *);
    }
    va_end(args);

    assert(!strcmp(path, "/bin/sh"));
    assert(!strcmp(argv[0], "sh"));
    assert(!strcmp(argv[1], "-c"));
    assert(argv[2] != NULL);
    assert(argv[3] == NULL);
    new_args = parse_arguments(argv[2], &argc);
    wai_getModulePath(lib, 1024, &length);
    sprintf(ldpreload, "%s/libexec-helper.so", dirname(lib));
    sprintf(ldlibexec, "%s/libxkbcomp.so", dirname(lib));
    setenv("LD_PRELOAD", ldpreload, 1);
    setenv("LD_LIBRARY_PATH", dirname(lib), 1);
    setenv("LD_LIBEXEC", ldlibexec, 1);

    __android_log_print(ANDROID_LOG_DEBUG, "LorieNative", "executing xkbcomp...");
    __android_log_print(ANDROID_LOG_DEBUG, "LorieNative", "ldlibpath %s (%d)", dirname(lib), getpid());
    __android_log_print(ANDROID_LOG_DEBUG, "LorieNative", "ldpreload %s", ldpreload);
    __android_log_print(ANDROID_LOG_DEBUG, "LorieNative", "ldlibexec %s", ldlibexec);

    for(int j=1; j<= SIGUNUSED; j++)
        signal(j, SIG_DFL);

#if defined(__i386__) || defined(__arm__)
    execv("/system/bin/app_process32", new_args);
#endif
    execv("/system/bin/sh", new_args);
    return -1;
}
