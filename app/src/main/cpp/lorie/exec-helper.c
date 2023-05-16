#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>

typedef int (*main_t)(int, char**, char**);
typedef void (*void_t)(void);

static int main_hook(int argc, char **argv, char **envp)
{
    void *h = dlopen(getenv("LD_LIBEXEC"), RTLD_NOW);
    if (!h) {
        dprintf(2, "Failed to dlopen %s: %s\n", getenv("LD_LIBEXEC"), dlerror());
        return 1;
    }

    int (*main)(int, char**, char**) = dlsym(h, "main");
    if (!main) {
        dprintf(2, "Failed to dlsym main: %s", dlerror());
        return 1;
    }

    return main(argc, argv, envp);
}

__attribute__((__used__))
int __libc_start_main(main_t main, int argc, char **argv, main_t init, void_t f, void_t *rf, void *se) {
    int (*orig)(main_t, int, char**, main_t, void_t, void_t, void*);
    orig = dlsym(RTLD_NEXT, "__libc_start_main");
    return orig(main_hook, argc, argv, init, f, rf, se);
}

__attribute__((__used__))
void __libc_init(void* raw_args, void (*onexit)(void), main_t main, void* s) {
    void (*orig)(void*, void(*)(void), int(*)(int, char**, char**), void*);
    orig = dlsym(RTLD_NEXT, "__libc_init");
    return orig(raw_args, onexit, main_hook, s);
}