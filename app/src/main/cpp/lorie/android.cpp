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
#include <xcb.h>
#include <linux/un.h>
#include <libgen.h>
#include "renderer.h"
#include "lorie.h"
#include "android-to-linux-keycodes.h"
#include "whereami.h"
#include "tx11.h"

#define DE_RESET     1
#define DE_TERMINATE 2
#define DE_PRIORITYCHANGE 4     /* set when a client's priority changes */

extern "C" {
    extern int dix_main(int argc, char *argv[], char *envp[]);
    extern const char *LogInit(const char *fname, const char *backup);
    extern int SetNotifyFd(int fd, void (*notify)(int,int,void*), int mask, void *data);
    extern int AddClientOnOpenFD(int fd);
    extern void AutoResetServer(int sig);
    extern int LogSetParameter(int param, int value);
    extern const char *XkbBaseDirectory;

    extern volatile char isItTimeToYield;
    extern volatile char dispatchException;
    extern void TimerCancel(void* timer);
    extern void* TimerSet(void*, int, uint32_t, uint32_t(*) (void*, uint32_t, void*), void*);

    extern char *__progname; // NOLINT(bugprone-reserved-identifier)
}

static char* progname() {
    static char* progname = nullptr;

    if (!progname) {
        progname = strdup(__progname);
        if (strstr(progname, ":"))
            strstr(progname, ":")[0] = 0;
    }

    return progname;
}

extern "C"
void NotifyDdx(void) {
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_termux_x11_CmdEntryPoint_start(maybe_unused JNIEnv *env, maybe_unused jclass clazz, maybe_unused jobjectArray args) {
    // execv's argv array is a bit incompatible with Java's String[], so we do some converting here...
    int argc = env->GetArrayLength(args) + 1; // Leading executable path
    char **argv = (char**) calloc(argc, sizeof(char*));

    argv[0] = (char*) "Xlorie";
    for(int i=1; i<argc; i++) {
        auto js = reinterpret_cast<jstring>(env->GetObjectArrayElement(args, i - 1));
        const char *pjc = env->GetStringUTFChars(js, JNI_FALSE);
        argv[i] = (char *) calloc(strlen(pjc) + 1, sizeof(char)); //Extra char for the terminating NULL
        strcpy((char *) argv[i], pjc);
        env->ReleaseStringUTFChars(js, pjc);
    }

    if (strstr(progname(), "com.termux")) {
        char root[128] = {0}, tmpdir[128] = {0}, lib[128] = {0}, apk[1024] = {0}, cmd[1024] = {0};

        asprintf((char **)(&XkbBaseDirectory), "/data/data/%s/files/usr/share/X11/xkb", progname());
        sprintf(root, "/data/data/%s/files", progname());
        sprintf(tmpdir, "/data/data/%s/files/", progname());
        sprintf(lib, "/data/data/%s/lib", progname());

        wai_getModulePath(apk, 1024, nullptr);
        if (strstr(apk, "!/lib"))
            strstr(apk, "!/lib")[0] = 0;
        sprintf(cmd, "unzip -d /data/data/com.termux.x11/files/ %s assets/xkb.tar", apk);
        system(cmd);
        system("mv /data/data/com.termux.x11/files/assets/xkb.tar /data/data/com.termux.x11/files/assets/xkb.tar.gz");
        system("mkdir -p /data/data/com.termux.x11/files/usr/share/X11/");
        system("tar -xf /data/data/com.termux.x11/files/assets/xkb.tar.gz -C /data/data/com.termux.x11/files/usr/share/X11/");
        setenv("XKB_CONFIG_ROOT", "/data/data/com.termux.x11/files/usr/share/X11/xkb", 1);

        LogSetParameter(2, 10);
        setenv("HOME", root, 1);
        setenv("TMPDIR", tmpdir, 1);
        setenv("XKB_CONFIG_ROOT", XkbBaseDirectory, 1);
        setenv("LIBGL_DRIVERS_PATH", lib, 1);
    } else {
        char lib[128] = {0};
        if (!getenv("TMPDIR")) {
            if (access("/tmp", F_OK) == 0)
                setenv("TMPDIR", "/tmp", 1);
            else if (access("/data/data/com.termux/files/usr/tmp", F_OK) == 0)
                setenv("TMPDIR", "/data/data/com.termux/files/usr/tmp", 1);
        }

        if (!getenv("TMPDIR")) {
            char* error = (char*) "$TMPDIR is not set. Normally it is pointing to /tmp of a container.";
            __android_log_print(ANDROID_LOG_ERROR, "LorieNative", "%s", error);
            dprintf(2, "%s\n", error);
            return JNI_FALSE;
        }

        if (!getenv("XKB_CONFIG_ROOT")) {
            if (access("/usr/share/X11/xkb", F_OK) == 0)
                setenv("XKB_CONFIG_ROOT", "/usr/share/X11/xkb", 1);
            else if (access("/data/data/com.termux/files/usr/share/X11/xkb", F_OK) == 0)
                setenv("XKB_CONFIG_ROOT", "/data/data/com.termux/files/usr/share/X11/xkb", 1);

            XkbBaseDirectory = getenv("XKB_CONFIG_ROOT");
        }

        if (!getenv("XKB_CONFIG_ROOT")) {
            char* error = (char*) "$XKB_CONFIG_ROOT is not set. Normally it is pointing to /usr/share/X11/xkb of a container.";
            __android_log_print(ANDROID_LOG_ERROR, "LorieNative", "%s", error);
            dprintf(2, "%s\n", error);
            return JNI_FALSE;
        }

        wai_getModulePath(lib, sizeof(lib), nullptr);
        setenv("LIBGL_DRIVERS_PATH", dirname(lib), 1);
    }

    if (access(XkbBaseDirectory, F_OK) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, "LorieNative", "%s is unaccessible: %s\n", XkbBaseDirectory, strerror(errno));
        printf("%s is unaccessible: %s\n", XkbBaseDirectory, strerror(errno));
        return JNI_FALSE;
    }

    std::thread([=] {
        char* envp[] = { nullptr };
        dix_main(argc, (char**) argv, envp);
    }).detach();
    return JNI_TRUE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_CmdEntryPoint_windowChanged(maybe_unused JNIEnv *env, maybe_unused jobject thiz, jobject surface) {
    ANativeWindow* win = surface ? ANativeWindow_fromSurface(env, surface) : nullptr;
    if (win)
        ANativeWindow_acquire(win);
    __android_log_print(ANDROID_LOG_INFO, "XlorieNative", "window change: %p", win);

    TimerSet(nullptr, 0, 1, [] (void* timer, uint32_t time, void* arg) -> uint32_t {
        lorieChangeWindow((struct ANativeWindow *) arg);
        TimerCancel(timer);
        return 0;
    }, (void*) win);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_termux_x11_CmdEntryPoint_getXConnection(JNIEnv *env, maybe_unused jobject thiz) {
    int client[2];
    jclass ParcelFileDescriptorClass = env->FindClass("android/os/ParcelFileDescriptor");
    jmethodID adoptFd = env->GetStaticMethodID(ParcelFileDescriptorClass, "adoptFd", "(I)Landroid/os/ParcelFileDescriptor;");

    socketpair(AF_UNIX, SOCK_STREAM, 0, client);
    TimerSet(nullptr, 0, 1, [] (void* timer, uint32_t time, void* arg) -> uint32_t {
        AddClientOnOpenFD((int) (int64_t) arg);
        TimerCancel(timer);
        return 0;
    }, (void*) (int64_t) client[1]);

    return env->CallStaticObjectMethod(ParcelFileDescriptorClass, adoptFd, client[0]);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_termux_x11_CmdEntryPoint_getLogcatOutput(JNIEnv *env, maybe_unused jobject thiz) {
    jclass ParcelFileDescriptorClass = env->FindClass("android/os/ParcelFileDescriptor");
    jmethodID adoptFd = env->GetStaticMethodID(ParcelFileDescriptorClass, "adoptFd", "(I)Landroid/os/ParcelFileDescriptor;");
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
        return env->CallStaticObjectMethod(ParcelFileDescriptorClass, adoptFd, p[1]);
    }
    return nullptr;
}

xcb_connection_t* conn{};

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_connect(maybe_unused JNIEnv *env, maybe_unused jobject thiz, jint fd) {
    xcb_connection_t* new_conn = xcb_connect_to_fd(fd, nullptr);
    int conn_err = xcb_connection_has_error(new_conn);
    if (conn_err) {
        const char *s;
        switch (conn_err) {
#define c(name) case name: s = static_cast<const char*>(#name); break
            c(XCB_CONN_ERROR);
            c(XCB_CONN_CLOSED_EXT_NOTSUPPORTED);
            c(XCB_CONN_CLOSED_MEM_INSUFFICIENT);
            c(XCB_CONN_CLOSED_REQ_LEN_EXCEED);
            c(XCB_CONN_CLOSED_PARSE_ERR);
            c(XCB_CONN_CLOSED_INVALID_SCREEN);
            c(XCB_CONN_CLOSED_FDPASSING_FAILED);
            default:
                s = "UNKNOWN";
#undef c
        }

        __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client", "XCB connection has error: %s", s);
    }

    if (conn)
        xcb_disconnect(conn);
    conn = new_conn;

    xcb_tx11_query_version(conn, 0, 1);
    __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client", "XCB connection successfull");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_startLogcat(maybe_unused JNIEnv *env, maybe_unused jobject thiz, maybe_unused jint fd) {
    // TODO: implement startLogcat()
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_sendWindowChange(maybe_unused JNIEnv *env, maybe_unused jobject thiz, jint width, jint height, jint dpi) {
    if (conn) {
        __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client", "Sending window changed: %d %d %d", width, height, dpi);
        xcb_tx11_screen_size_change(conn, width, height, dpi);
        xcb_flush(conn);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_sendMouseEvent(maybe_unused JNIEnv *env, maybe_unused jobject thiz,
                                                jint x, jint y, jint which_button, jboolean button_down, jboolean relative) {
    if (conn) {
        __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client", "Sending mouse event: %d %d %d %d %d", x, y, which_button, button_down, relative);
        xcb_tx11_mouse_event(conn, x, y, which_button, button_down, relative); // NOLINT(cppcoreguidelines-narrowing-conversions)
        xcb_flush(conn);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_sendTouchEvent(maybe_unused JNIEnv *env, maybe_unused jobject thiz, jint action, jint id, jint x, jint y) {
    if (conn) {
        if (action >= 0) {
            __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client","Sending touch event: %d %d %d %d", action, id, x, y);
            xcb_tx11_touch_event(conn, action, id, x, y); // NOLINT(cppcoreguidelines-narrowing-conversions)
        } else xcb_flush(conn);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_sendKeyEvent(maybe_unused JNIEnv *env, maybe_unused jobject thiz, jint scan_code, jint key_code, jboolean key_down) {
    if (conn) {
        int code = (scan_code) ?: android_to_linux_keycode[key_code];
        __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client","Sending key event: %d %d %d %s", scan_code, key_code, code, key_down ? "press" : "release");
        xcb_tx11_key_event(conn, code + 8, key_down);
        xcb_flush(conn);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_sendUnicodeEvent(maybe_unused JNIEnv *env, maybe_unused jobject thiz, jint unicode) {
    if (conn) {
        __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client","Sending unicode event: %d", unicode);
        xcb_tx11_unicode_event(conn, unicode);
        xcb_flush(conn);
    }
}

static JavaVM *vm;
static jclass system_cls;
static jmethodID exit_mid;

jint JNI_OnLoad(JavaVM *jvm, maybe_unused void *reserved) {
    vm = jvm;
    JNIEnv* env;
    vm->AttachCurrentThread(&env, nullptr);
    system_cls = (jclass) env->NewGlobalRef(env->FindClass("java/lang/System"));
    exit_mid = env->GetStaticMethodID(system_cls, "exit", "(I)V");
    return JNI_VERSION_1_6;
}

extern "C"
void abort() {
    JNIEnv* env;
    vm->AttachCurrentThread(&env, nullptr);
    env->FatalError("aborted");
    while(true)
        sleep(1);
}

extern "C"
void exit(int code) {
    JNIEnv* env;
    vm->AttachCurrentThread(&env, nullptr);
    env->FatalError("exited");
    env->CallStaticVoidMethod(system_cls, exit_mid, code);
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

unused no_instrument __cyg_profile_func_enter (void *func, maybe_unused void *caller) { // NOLINT(bugprone-reserved-identifier)
    level++;
    print_func(func, 1);
}

unused no_instrument __cyg_profile_func_exit (void *func, maybe_unused void *caller) { // NOLINT(bugprone-reserved-identifier)
    print_func(func, 0);
    level--;
}

} // extern "C"
#endif


// I need this to catch initialisation errors of libxkbcommon.
#if 1
// It is needed to redirect stderr to logcat
maybe_unused std::thread stderr_to_logcat_thread([]{ // NOLINT(cert-err58-cpp,cppcoreguidelines-interfaces-global-init)
    if (!strcmp("com.termux.x11", progname())) {
        FILE *fp;
        int p[2];
        size_t len;
        char *line = nullptr;
        pipe(p);

        fp = fdopen(p[0], "r");

        dup2(p[1], 2);
        dup2(p[1], 1);
        while ((getline(&line, &len, fp)) != -1) {
            __android_log_print(ANDROID_LOG_VERBOSE, "stderr", "%s%s", line,
                                (line[len - 1] == '\n') ? "" : "\n");
        }
    }
});
#endif
