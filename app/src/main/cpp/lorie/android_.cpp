#pragma clang diagnostic ignored "-Wmissing-prototypes"
#include <thread>
#include <jni.h>
#include <dlfcn.h>     // for dladdr
#include <cxxabi.h>    // for __cxa_demangle
#include <android/log.h>
#include <android/native_window_jni.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <libgen.h>
#include "lorie_looper.hpp"
#include "renderer.h"
#include "egw.h"
#include "android-to-linux-keycodes.h"
#include "whereami.c"

lorie_looper looper;

extern "C" {
    extern int LogSetParameter(int param, int value);
    extern const char *XkbBaseDirectory;
    extern int AddClientOnOpenFD(int fd);
}

void egw_init(void) {
    SetNotifyFd(looper.getfd(), [](int,int,void*) {
        looper.dispatch(0);
    }, 1, nullptr);
}

extern "C" JNIEXPORT void JNICALL
Java_com_termux_x11_CmdEntryPoint_start(JNIEnv *env, unused jclass clazz, jobjectArray args) {
    // execv's argv array is a bit incompatible with Java's String[], so we do some converting here...
    int argc = env->GetArrayLength(args) + 2; // Leading executable path and terminating NULL
    char **argv = (char**) calloc(argc, sizeof(char*));
    memset(argv, 0, sizeof(char*) * argc);

    argv[0] = (char*) "Xlorie";
    argv[argc-1] = nullptr; // Terminating NULL
    for(int i=1; i<argc-1; i++) {
        auto js = reinterpret_cast<jstring>(env->GetObjectArrayElement(args, i - 1));
        const char *pjc = env->GetStringUTFChars(js, JNI_FALSE);
        argv[i] = static_cast<char *>(calloc(strlen(pjc) + 1, sizeof(char))); //Extra char for the terminating NULL
        strcpy((char *) argv[i], pjc);
        env->ReleaseStringUTFChars(js, pjc);
    }

    XkbBaseDirectory = nullptr;

    char apk[1024], cmd[1024];
    wai_getModulePath(apk, 1024, nullptr);
    if (strstr(apk, "!/lib"))
        strstr(apk, "!/lib")[0] = 0;
    sprintf(cmd, "unzip -d /data/data/com.termux.x11/files/ %s assets/xkb.tar", apk);
    system(cmd);
    system("mv /data/data/com.termux.x11/files/assets/xkb.tar /data/data/com.termux.x11/files/assets/xkb.tar.gz");
    system("mkdir -p /data/data/com.termux.x11/files/usr/share/X11/");
    system("tar -xf /data/data/com.termux.x11/files/assets/xkb.tar.gz -C /data/data/com.termux.x11/files/usr/share/X11/");
    setenv("XKB_CONFIG_ROOT", "/data/data/com.termux.x11/files/usr/share/X11/xkb", 1);

    if (!getenv("TMPDIR") && (access("/tmp", F_OK) == 0))
        setenv("TMPDIR", "/tmp", 1);
    if (!getenv("TMPDIR") && (access("/data/data/com.termux/files/usr/tmp", F_OK) == 0))
        setenv("TMPDIR", "/data/data/com.termux/files/usr/tmp", 1);
    if (access("/usr/share/X11/xkb", F_OK) == 0)
        XkbBaseDirectory = "/usr/share/X11/xkb";
    if (!XkbBaseDirectory && (access(getenv("XKB_CONFIG_ROOT"), F_OK) == 0))
        XkbBaseDirectory = getenv("XKB_CONFIG_ROOT");
    if (!XkbBaseDirectory)
        XkbBaseDirectory = "/data/data/com.termux/files/usr/share/X11/xkb";
    setenv("XKB_CONFIG_ROOT", XkbBaseDirectory, 1);

    char lib[1024];
    int length;
    wai_getModulePath(lib, 1024, &length);
    setenv("LIBGL_DRIVERS_PATH", dirname(lib), 1);

    LogSetParameter(2, 10);

    std::thread([=] {
        char* envp[] = { nullptr };
        renderer_init();
        dix_main(argc-1, argv, envp);
    }).detach();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_termux_x11_CmdEntryPoint_connect(unused JNIEnv *env, unused jclass clazz) {
    int fd[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fd);

    looper.post([=] {
        AddClientOnOpenFD(fd[0]);
        //close(fd[1]);
    }, 200);

    return fd[1];
}

extern "C" JNIEXPORT jint JNICALL
Java_com_termux_x11_CmdEntryPoint_stderr(maybe_unused JNIEnv *env, maybe_unused jclass clazz) {
    const char *debug = getenv("TERMUX_X11_DEBUG");
    if (debug && !strcmp(debug, "1")) {
        int p[2];
        pipe(p);
        fchmod(p[1], 0777);
        std::thread([=] {
            char buffer[4096];
            size_t len;
            while((len = read(p[0], buffer, 4096)) >=0)
                write(2, buffer, len);
            close(p[0]);
        }).detach();
        return p[1];
    }

    return -1;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_termux_x11_CmdEntryPoint_getuid(maybe_unused JNIEnv *env, maybe_unused jclass clazz) {
    return getuid(); // NOLINT(cppcoreguidelines-narrowing-conversions)
}

extern "C" JNIEXPORT void JNICALL
Java_com_termux_x11_CmdEntryPoint_windowChanged(JNIEnv *env, unused jobject thiz, jobject surface,
                                                jint width, jint height, jint dpi) {
    ANativeWindow* win = surface ? ANativeWindow_fromSurface(env, surface) : nullptr;
    if (win)
        ANativeWindow_acquire(win);
    __android_log_print(ANDROID_LOG_INFO, "XegwNative", "window change: %p", win);

    looper.post([=] {
        vfbConfigureNotify(win, width, height, dpi);
    });
}

#if 0
extern "C"
JNIEXPORT void JNICALL
Java_com_eltechs_axs_xserver_RealXServer_windowChanged(JNIEnv *env, unused jclass clazz, jobject surface, jint width, jint height) {
    ANativeWindow* win = surface ? ANativeWindow_fromSurface(env, surface) : nullptr;
    if (win)
        ANativeWindow_acquire(win);
    __android_log_print(ANDROID_LOG_INFO, "XegwNative", "window change: %p", win);

    looper.post([=] {
        vfbWindowChange(win);
        vfbConfigureNotify(width, height);
    });
}
#endif

static JavaVM *vm;

jint JNI_OnLoad(JavaVM *jvm, maybe_unused void *reserved) {
    vm = jvm;
    return JNI_VERSION_1_6;
}

extern "C"
void abort() {
    JNIEnv* env;
    vm->AttachCurrentThread(&env, nullptr);
    jclass systemCls = env->FindClass("java/lang/System");
    jmethodID exitMID = env->GetStaticMethodID(systemCls, "exit", "(I)V");
    env->CallStaticVoidMethod(systemCls, exitMID, 0);
    while(true)
        sleep(1);
}

extern "C"
void exit(int code) {
    JNIEnv* env;
    vm->AttachCurrentThread(&env, nullptr);
    jclass systemCls = env->FindClass("java/lang/System");
    jmethodID exitMID = env->GetStaticMethodID(systemCls, "exit", "(I)V");
    env->CallStaticVoidMethod(systemCls, exitMID, code);
    while(true)
        sleep(1);
}

#if 0
bool enabled = true;
#define no_instrument void __attribute__((no_instrument_function)) __attribute__ ((visibility ("default")))

using namespace std;
extern "C" {
static thread_local int level = -1;
no_instrument print_func(void *func, int enter) {
    int status;
    Dl_info info;
    if (!dladdr(func, &info) || !info.dli_sname)
        return;

    char *demangled = abi::__cxa_demangle(info.dli_sname,nullptr, nullptr, &status);
    __android_log_print(ANDROID_LOG_DEBUG, "LorieProfile", "%d%*c%s %s", gettid(), level, ' ', enter ? ">" : "<", status == 0 ? demangled : info.dli_sname);
    free(demangled);
}

unused no_instrument __cyg_profile_func_enter (void *func, [[maybe_unused]] void *caller) { // NOLINT(bugprone-reserved-identifier)
    level++;
    print_func(func, 1);
}

unused no_instrument __cyg_profile_func_exit (void *func, [[maybe_unused]] void *caller) { // NOLINT(bugprone-reserved-identifier)
    print_func(func, 0);
    level--;
}

} // extern "C"
#endif
