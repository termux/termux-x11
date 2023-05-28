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
#include <xcb/xfixes.h>
#include <xcb_errors.h>
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
    extern const char *defaultFontPath;

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
Java_com_termux_x11_CmdEntryPoint_start(JNIEnv *env, jclass, jobjectArray args) {
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

    {
        const char *root_dir = dirname(getenv("TMPDIR"));
        const char* pathes[] = {
                "/etc/X11/fonts", "/usr/share/fonts/X11", "/share/fonts", nullptr
        };
        for (int i=0; pathes[i]; i++) {
            char current_path[1024] = {0};
            snprintf(current_path, sizeof(current_path), "%s%s", root_dir, pathes[i]);
            if (access(current_path, F_OK) == 0) {
                char default_font_path[4096] = {0};
                snprintf(default_font_path, sizeof(default_font_path),
                         "%s/misc,%s/TTF,%s/OTF,%s/Type1,%s/100dpi,%s/75dpi",
                         current_path, current_path, current_path, current_path, current_path, current_path);
                defaultFontPath = strdup(default_font_path);
                break;
            }
        }
    }

    if (!getenv("XKB_CONFIG_ROOT")) {
        if (access("/usr/share/X11/xkb", F_OK) == 0)
            setenv("XKB_CONFIG_ROOT", "/usr/share/X11/xkb", 1);
        else if (access("/data/data/com.termux/files/usr/share/X11/xkb", F_OK) == 0)
            setenv("XKB_CONFIG_ROOT", "/data/data/com.termux/files/usr/share/X11/xkb", 1);
    }

    if (!getenv("XKB_CONFIG_ROOT")) {
        char* error = (char*) "$XKB_CONFIG_ROOT is not set. Normally it is pointing to /usr/share/X11/xkb of a container.";
        __android_log_print(ANDROID_LOG_ERROR, "LorieNative", "%s", error);
        dprintf(2, "%s\n", error);
        return JNI_FALSE;
    }

    wai_getModulePath(lib, sizeof(lib), nullptr);
    setenv("LIBGL_DRIVERS_PATH", dirname(lib), 1);

    XkbBaseDirectory = getenv("XKB_CONFIG_ROOT");
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
Java_com_termux_x11_CmdEntryPoint_windowChanged(JNIEnv *env, jobject, jobject surface) {
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
Java_com_termux_x11_CmdEntryPoint_getXConnection(JNIEnv *env, jobject) {
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
Java_com_termux_x11_CmdEntryPoint_getLogcatOutput(JNIEnv *env, jobject) {
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

static xcb_connection_t* conn{};
static int xfixes_first_event{};
static xcb_errors_context_t *err_ctx{};
static xcb_window_t win{};
static xcb_atom_t prop_sel{};
static xcb_atom_t atom_clipboard{};
static bool clipboard_sync_enabled = false;

static void parse_error(xcb_generic_error_t* err) {
    if (!err)
        return;
    const char* ext{};
    const char* err_name =  xcb_errors_get_name_for_error(err_ctx, err->error_code, &ext);
    std::string err_text = std::string("") +
                           "XCB Error of failed request:               " + (ext?:"") + "::" + err_name + "\n" +
                           "  Major opcode of failed request:          " + std::to_string(err->major_code) + " (" +
                           xcb_errors_get_name_for_major_code(err_ctx, err->major_code) + ")\n" +
                           "  Minor opcode of failed request:          " + std::to_string(err->minor_code) + " (" +
                           (xcb_errors_get_name_for_minor_code(err_ctx, err->major_code, err->minor_code) ?: "Core") + ")\n" +
                           "  Serial number of failed request:         " + std::to_string(err->sequence) + "\n" +
                           "  Current serial number in output stream:  " + std::to_string(err->full_sequence);
    __android_log_print(ANDROID_LOG_ERROR, "LorieNative", "%s", err_text.c_str());
    free(err);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_connect(JNIEnv*, jobject, jint fd) {
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

    if (err_ctx)
        xcb_errors_context_free(err_ctx);
    if (conn)
        xcb_disconnect(conn);
    conn = new_conn;
    xcb_errors_context_new(conn, &err_ctx);

    __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client", "XCB connection is successfull");

    xcb_generic_error_t* err{};

    {
        auto cookie = xcb_query_extension(conn, 6, "XFIXES");
        auto reply = xcb_query_extension_reply(conn, cookie, &err);
        parse_error(err);
        if (reply)
            xfixes_first_event = reply->first_event;
        free(reply);
    }
    {
        auto cookie = xcb_xfixes_query_version(conn, 4, 0);
        auto reply = xcb_xfixes_query_version_reply(conn, cookie, &err);
        parse_error(err);
        free(reply);
    }
    {
        auto cookie = xcb_intern_atom(conn, false, 9, "CLIPBOARD");
        auto reply = xcb_intern_atom_reply(conn, cookie, &err);
        parse_error(err);
        if (reply)
            atom_clipboard = reply->atom;
        free(reply);
    }
    {
        auto cookie = xcb_intern_atom(conn, false, 15, "TERMUX_X11_CLIP");
        auto reply = xcb_intern_atom_reply(conn, cookie, &err);
        parse_error(err);
        if (reply)
            prop_sel = reply->atom;
        free(reply);
    }
    {
        xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
        auto cookie = xcb_xfixes_select_selection_input_checked(conn, s->root, atom_clipboard,
                                                        XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER);
        parse_error(xcb_request_check(conn, cookie));
    }

    {
        xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
        win = xcb_generate_id(conn);
        auto cookie = xcb_create_window_checked(conn, 0, win, s->root, 0, 0,
                                                10, 10, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
                                                XCB_COPY_FROM_PARENT, XCB_CW_OVERRIDE_REDIRECT,
                                                (const uint32_t[]) {true});
        parse_error(xcb_request_check(conn, cookie));
    }

    xcb_flush(conn);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_handleXEvents(JNIEnv *env, jobject thiz) {
#if 0
    static jfieldID fid = nullptr;
    if (!fid) {
        jclass fdClass = env->FindClass("java/io/FileDescriptor");
        fid = fdClass ? env->GetFieldID(fdClass, "descriptor", "I") : nullptr;
        if (fid == nullptr) {
            __android_log_print(ANDROID_LOG_ERROR, "LorieNative",
                                "Failed to find java/io/FileDescriptor class");
            return 0;
        }
    }

    int fd = env->GetIntField(jfd, fid);

    // Other file descriptor means there is new connection and old one is not needed.
    // Looper will automatically close other file descriptor, so there is no need to take care of this.
    if (conn && xcb_get_file_descriptor(conn) != fd)
        return 0;

    if ((events & 4) == 4) // Catching EVENT_ERROR
        return 0;
#endif

    if (!conn)
        return;

    xcb_generic_event_t *ev;
    while ((ev = xcb_poll_for_event(conn))) {
        __android_log_print(ANDROID_LOG_ERROR, "LorieNative",
                            "Response is 0x%X!", ev->response_type);
        if (!ev->response_type)
            parse_error((xcb_generic_error_t *) ev);
        else if (ev->response_type == xfixes_first_event + XCB_XFIXES_SELECTION_NOTIFY) { // Clipboard selection notify
            __android_log_print(ANDROID_LOG_ERROR, "LorieNative",
                                "Clipboard content changed!");
            if (clipboard_sync_enabled) {
                xcb_convert_selection(conn, win, atom_clipboard,XCB_ATOM_STRING, prop_sel, XCB_CURRENT_TIME);
                xcb_flush(conn);
            }

        } else if ((ev->response_type & ~0x80) == XCB_SELECTION_NOTIFY) { // Clipboard selection response
            jmethodID id = env->GetMethodID(env->GetObjectClass(thiz), "setClipboardText","(Ljava/lang/String;)V");
            xcb_generic_error_t* err{};
            uint32_t data_size = 0;
            __android_log_print(ANDROID_LOG_INFO, "LorieNative", "We've got selection (clipboard) contents");
            {
                auto cookie = xcb_get_property(conn, false, win, prop_sel, XCB_ATOM_ANY, 0, 0);
                auto reply = xcb_get_property_reply(conn, cookie, &err);
                parse_error(err);
                if (reply)
                    data_size = reply->bytes_after;
                free(reply);
            }
            __android_log_print(ANDROID_LOG_INFO, "LorieNative", "Clipboard size is %d", data_size);
            if (data_size) {
                auto cookie = xcb_get_property(conn, false, win, prop_sel, XCB_ATOM_ANY, 0, data_size);
                auto reply = xcb_get_property_reply(conn, cookie, &err);
                parse_error(err);
                if (reply)
                    env->CallVoidMethod(thiz, id, env->NewStringUTF((const char *) xcb_get_property_value(reply)));
                free(reply);
            }
        }
    }

//    return 5; // EVENT_INPUT | EVENT_ERROR = 5
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_startLogcat(JNIEnv *env, jobject, jint fd) {
    kill(-1, SIGTERM); // Termux:X11 starts only logcat, so any other process should not be killed.

    __android_log_print(ANDROID_LOG_DEBUG, "LorieNative", "Starting logcat with output to given fd");
    system("/system/bin/logcat -c");

    switch(fork()) {
        case -1:
            __android_log_print(ANDROID_LOG_DEBUG, "LorieNative", "fork: %s", strerror(errno));
            return;
        case 0:
            dup2(fd, 1);
            dup2(fd, 2);
            execl("/system/bin/logcat", "logcat", nullptr);
            __android_log_print(ANDROID_LOG_DEBUG, "LorieNative", "exec logcat: %s", strerror(errno));
            env->FatalError("Exiting");
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_setClipboardSyncEnabled(JNIEnv*, jobject, jboolean enabled) {
    clipboard_sync_enabled = enabled;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_sendWindowChange(JNIEnv*, jobject, jint width, jint height) {
    if (conn) {
//        __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client", "Sending window changed: %d %d %d", width, height, dpi);
        xcb_tx11_screen_size_change(conn, width, height);
        xcb_flush(conn);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_sendMouseEvent(JNIEnv*, jobject, jfloat x, jfloat y, jint which_button, jboolean button_down, jboolean relative) {
    if (conn) {
        __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client", "Sending mouse event: %f %f %d %d %d", x, y, which_button, button_down, relative);
        xcb_tx11_mouse_event(conn, x, y, which_button, button_down, relative); // NOLINT(cppcoreguidelines-narrowing-conversions)
        xcb_flush(conn);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_sendTouchEvent(JNIEnv*, jobject, jint action, jint id, jint x, jint y) {
    if (conn) {
        if (action >= 0) {
//            __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client","Sending touch event: %d %d %d %d", action, id, x, y);
            xcb_tx11_touch_event(conn, action, id, x, y); // NOLINT(cppcoreguidelines-narrowing-conversions)
        } else xcb_flush(conn);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_sendKeyEvent(JNIEnv*, jobject, jint scan_code, jint key_code, jboolean key_down) {
    if (conn) {
        int code = (scan_code) ?: android_to_linux_keycode[key_code];
//        __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client","Sending key event: %d %d %d %s", scan_code, key_code, code, key_down ? "press" : "release");
        xcb_tx11_key_event(conn, code + 8, key_down);
        xcb_flush(conn);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_sendUnicodeEvent(JNIEnv*, jobject, jint unicode) {
    if (conn) {
//        __android_log_print(ANDROID_LOG_ERROR, "Xlorie-client","Sending unicode event: %d", unicode);
        xcb_tx11_unicode_event(conn, unicode);
        xcb_flush(conn);
    }
}

static JavaVM *vm;
static jclass system_cls;
static jmethodID exit_mid;

jint JNI_OnLoad(JavaVM *jvm, unused void *reserved) {
    vm = jvm;
    JNIEnv* env;
    vm->AttachCurrentThread(&env, nullptr);
    system_cls = (jclass) env->NewGlobalRef(env->FindClass("java/lang/System"));
    exit_mid = env->GetStaticMethodID(system_cls, "exit", "(I)V");
    init_module();
    return JNI_VERSION_1_6;
}

extern "C"
void abort() {
    JNIEnv* env;
    vm->AttachCurrentThread(&env, nullptr);
    env->CallStaticVoidMethod(system_cls, exit_mid, 134);
    while(true)
        sleep(1);
}

extern "C"
void exit(int code) {
    JNIEnv* env;
    vm->AttachCurrentThread(&env, nullptr);
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

unused no_instrument __cyg_profile_func_enter (void *func, void*) { // NOLINT(bugprone-reserved-identifier)
    level++;
    print_func(func, 1);
}

unused no_instrument __cyg_profile_func_exit (void *func, void*) { // NOLINT(bugprone-reserved-identifier)
    print_func(func, 0);
    level--;
}

} // extern "C"
#endif


// I need this to catch initialisation errors of libxkbcommon.
#if 1
// It is needed to redirect stderr to logcat
unused std::thread stderr_to_logcat_thread([]{ // NOLINT(cert-err58-cpp,cppcoreguidelines-interfaces-global-init)
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