#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <errno.h>
#include <jni.h>
#include <android/looper.h>
#include <wchar.h>
#include <linux/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <new>
#include "lorie.h"

#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#pragma ide diagnostic ignored "ConstantFunctionResult"
#define log(prio, ...) __android_log_print(ANDROID_LOG_ ## prio, "LorieNative", __VA_ARGS__)
// `r` must be a LorieViewResources* — the connection fd is per-instance state, not a process global.
#define sendEvent(r, ...) do { if ((r) && (r)->connFd != -1) { lorieEvent e = { __VA_ARGS__ }; write((r)->connFd, &e, sizeof(e)); } } while (0)

bool lorieDebugEnabled = false;

// Timestamp of the last real input reaching the X session, from any source. Read/written only
// from the Android main thread via JNI.
static volatile int64_t lastInputTimestampMs = 0;

static int64_t nowMs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static struct {
    jclass self;
    jmethodID getInstance, clientConnectedStateChanged, resetIme;
} MainActivity = {0};

static struct {
    jclass self;
    jmethodID forName;
    jmethodID decode;
} Charset = {0};

static struct {
    jclass self;
    jmethodID toString;
} CharBuffer = {0};

// Bundles the native state belonging to the current LorieView instance so it can be released
// as a unit when that instance is torn down, instead of leaking as scattered process globals.
struct LorieViewResources {
    Renderer renderer;
    JNIEnv* env = NULL;    // GUI-thread JNIEnv. Must be used only in GUI thread.
    jobject thiz = NULL;   // global ref to the owning LorieView
    volatile int connFd = -1;
    bool destroyed = true;

    LorieViewResources(JNIEnv* callerEnv, jobject view);
    ~LorieViewResources();
    void connect(jint fd);
    int xcallback(int fd, int events);
};

static jclass FindClassOrDie(JNIEnv *env, const char* name) {
    jclass clazz = env->FindClass(name);
    if (!clazz) {
        char buffer[1024] = {0};
        sprintf(buffer, "class %s not found", name);
        log(ERROR, "%s", buffer);
        env->FatalError(buffer);
        return NULL;
    }

    return (jclass) env->NewGlobalRef(clazz);
}

static jmethodID FindMethodOrDie(JNIEnv *env, jclass clazz, const char* name, const char* signature, jboolean isStatic) {
    jmethodID method = isStatic ? env->GetStaticMethodID(clazz, name, signature) : env->GetMethodID(clazz, name, signature);
    if (!method) {
        char buffer[1024] = {0};
        sprintf(buffer, "method %s %s not found", name, signature);
        log(ERROR, "%s", buffer);
        env->FatalError(buffer);
        return NULL;
    }

    return method;
}

static jboolean requestConnection(__unused jlong ptr) {
#define check(cond, fmt, ...) if ((cond)) do { __android_log_print(ANDROID_LOG_ERROR, "requestConnection", fmt, ## __VA_ARGS__); goto end; } while (0)
    bool sent = JNI_FALSE;
    // We do not want to block GUI thread for a long time so we will set timeout to 20 msec.
    struct sockaddr_in server = { .sin_family = AF_INET, .sin_port = htons(PORT) };
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    int so_error, sock = socket(AF_INET, SOCK_STREAM, 0);
    check(sock < 0, "Could not create socket: %s", strerror(errno));
    check(fcntl(sock, F_SETFL, O_NONBLOCK) < 0, "failed to set socket non-block: %s", strerror(errno));
    int r;
    r = connect(sock, (struct sockaddr *)&server, sizeof(server));
    check(r < 0 && errno != EINPROGRESS, "failed to connect socket: %s", strerror(errno));
    if (r < 0 && errno == EINPROGRESS) {
        // Connection is in progress; use poll to wait for it
        struct pollfd pfd = { .fd = sock, .events = POLLOUT };
        r = poll(&pfd, 1, 20);  // timeout set to 50ms
        if (!r) goto end;
        // check(!r, "Connection timed out after 20ms."); // We do not want to flood logcat with this message
        check(r < 0, "poll failed: %s", strerror(errno));
        socklen_t len = sizeof(so_error);
        check(getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0, "getsockopt failed: %s", strerror(errno));
        if (so_error == ECONNREFUSED) goto end; // Regular situation which happens often if server is not started. No need to spam logcat with this.
        check(so_error != 0, "Connection failed: %s", strerror(so_error));

        check(write(sock, MAGIC, sizeof(MAGIC)) < 0, "failed to send message: %s", strerror(errno));
        sent = JNI_TRUE;
        goto end;
    }

    check(1, "something went wrong: %s, %s", strerror(errno), strerror(r));

    end: if (sock >= 0) close(sock);
    return sent;
#undef errorReturn
}

static jlong nativeInit(JNIEnv *env, jobject thiz) {
    if (!Charset.self) {
        // Init clipboard-related JNI stuff
        Charset.self = FindClassOrDie(env, "java/nio/charset/Charset");
        Charset.forName = FindMethodOrDie(env, Charset.self, "forName", "(Ljava/lang/String;)Ljava/nio/charset/Charset;", JNI_TRUE);
        Charset.decode = FindMethodOrDie(env, Charset.self, "decode", "(Ljava/nio/ByteBuffer;)Ljava/nio/CharBuffer;", JNI_FALSE);

        CharBuffer.self = FindClassOrDie(env,  "java/nio/CharBuffer");
        CharBuffer.toString = FindMethodOrDie(env, CharBuffer.self, "toString", "()Ljava/lang/String;", JNI_FALSE);

        MainActivity.self = FindClassOrDie(env,  "com/termux/x11/MainActivity");
        MainActivity.getInstance = FindMethodOrDie(env, MainActivity.self, "getInstance", "()Lcom/termux/x11/MainActivity;", JNI_TRUE);
        MainActivity.clientConnectedStateChanged = FindMethodOrDie(env, MainActivity.self, "clientConnectedStateChanged", "()V", JNI_FALSE);
        MainActivity.resetIme = FindMethodOrDie(env, env->GetObjectClass(thiz), "resetIme", "()V", JNI_FALSE);
    }

    return (jlong) (intptr_t) new (malloc(sizeof(LorieViewResources))) LorieViewResources(env, thiz);
}

LorieViewResources::LorieViewResources(JNIEnv *callerEnv, jobject view) {
    JavaVM* vm;
    destroyed = false;
    renderer.init(callerEnv);
    renderer.connFdPtr = &connFd; // lets the renderer thread wake up a GPU copy waiter

    callerEnv->GetJavaVM(&vm);
    vm->AttachCurrentThread(&env, NULL);
    thiz = env->NewGlobalRef(view);
    connect(-1);
}

LorieViewResources::~LorieViewResources() {
    destroyed = true;

    if (connFd != -1) {
        ALooper_removeFd(ALooper_forThread(), connFd);
        close(connFd);
        connFd = -1;
    }

    renderer.destroy();

    if (thiz) {
        env->DeleteGlobalRef(thiz);
        thiz = NULL;
    }
}

int LorieViewResources::xcallback(int fd, int events) {
    if (events & (ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP)) {
        jobject instance = env->CallStaticObjectMethod(MainActivity.self, MainActivity.getInstance);
        if (instance)
            env->CallVoidMethod(instance, MainActivity.clientConnectedStateChanged);

        ALooper_removeFd(ALooper_forThread(), fd);
        close(connFd);
        connFd = -1;
        renderer.setSharedState(NULL);
        renderer.removeAllBuffers();
        log(DEBUG, "disconnected");
        return 1;
    }

    if (connFd != -1) {
        lorieEvent e = {0};

        again:
        if (read(connFd, &e, sizeof(e)) == sizeof(e)) {
            switch(e.type) {
                case EVENT_CLIPBOARD_SEND: {
                    if (!e.clipboardSend.count)
                        break;
                    char clipboard[e.clipboardSend.count + 1];
                    memset(clipboard, 0, e.clipboardSend.count + 1);
                    read(connFd, clipboard, sizeof(clipboard));
                    clipboard[e.clipboardSend.count] = 0;
                    log(DEBUG, "Clipboard content (%zu symbols) is %s", strlen(clipboard), clipboard);
                    jmethodID id = env->GetMethodID(env->GetObjectClass(thiz), "setClipboardText","(Ljava/lang/String;)V");
                    jobject bb = env->NewDirectByteBuffer(clipboard, strlen(clipboard));
                    jobject charset = env->CallStaticObjectMethod(Charset.self, Charset.forName, env->NewStringUTF("UTF-8"));
                    jobject cb = env->CallObjectMethod(charset, Charset.decode, bb);
                    env->DeleteLocalRef(bb);

                    jstring str = (jstring) env->CallObjectMethod(cb, CharBuffer.toString);
                    env->CallVoidMethod(thiz, id, str);
                    break;
                }
                case EVENT_CLIPBOARD_REQUEST: {
                    env->CallVoidMethod(thiz, env->GetMethodID(env->GetObjectClass(thiz), "requestClipboard", "()V"));
                    break;
                }
                case EVENT_SHARED_SERVER_STATE: {
                    struct lorie_shared_server_state* state = NULL;
                    int stateFd = ancil_recv_fd(connFd);

                    if (stateFd < 0)
                        break;

                    state = (struct lorie_shared_server_state*) mmap(NULL, sizeof(*state), PROT_READ|PROT_WRITE, MAP_SHARED, stateFd, 0);
                    if (!state || state == MAP_FAILED) {
                        log(ERROR, "Failed to map server state: %s", strerror(errno));
                        state = NULL;
                    }

                    renderer.setSharedState(state);

                    close(stateFd); // Closing file descriptor does not unmmap shared memory fragment.
                    break;
                }
                case EVENT_ADD_BUFFER: {
                    static LorieBuffer* buffer = NULL;
                    const LorieBuffer_Desc* desc;
                    LorieBuffer_recvHandleFromUnixSocket(connFd, &buffer);
                    desc = LorieBuffer_description(buffer);
                    log(INFO, "Received shared buffer width %d stride %d height %d format %d type %d id %llu", desc->width, desc->stride, desc->height, desc->format, desc->type, desc->id);
                    renderer.addBuffer(buffer);
                    break;
                }
                case EVENT_REMOVE_BUFFER: {
                    renderer.removeBuffer(e.removeBuffer.id);
                    break;
                }
                case EVENT_WINDOW_FOCUS_CHANGED: {
                    env->CallVoidMethod(thiz, MainActivity.resetIme);
                }
            }
        }

        int n;
        if (ioctl(connFd, FIONREAD, &n) >= 0 && n > sizeof(e))
            goto again;
    }

    return 1;
}

void LorieViewResources::connect(jint fd) {
    if (connFd != -1) {
        ALooper_removeFd(ALooper_forThread(), connFd);
        close(connFd);
        renderer.setSharedState(NULL);
        renderer.removeAllBuffers();
        log(DEBUG, "disconnected");
    }

    if ((connFd = fd) != -1) {
        ALooper_addFd(ALooper_forThread(), fd, 0, ALOOPER_EVENT_INPUT | ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP,
                      +[](int fd, int events, void* data) -> int { return ((LorieViewResources*) data)->xcallback(fd, events); }, this);

        // Give the X server our renderer wakeup cond var fd, resent on every reconnect.
        lorieEvent e = { .type = EVENT_RENDERER_WAKEUP_COND };
        write(connFd, &e, sizeof(e));
        ancil_send_fd(connFd, renderer.getWakeupCondFd());

        log(DEBUG, "XCB connection is successfull");
    }
}

static void startLogcat(JNIEnv *env, __unused jclass clazz, __unused jlong ptr, jint fd) {
    log(DEBUG, "Starting logcat with output to given fd");
    lorieDebugEnabled = true;

    switch(fork()) {
        case -1:
            log(ERROR, "fork: %s", strerror(errno));
            return;
        case 0:
            dup2(fd, 1);
            dup2(fd, 2);
            prctl(PR_SET_PDEATHSIG, SIGTERM);
            char buf[64] = {0};
            sprintf(buf, "--pid=%d", getppid());
            execl("/system/bin/logcat", "logcat", buf, NULL);
            log(ERROR, "exec logcat: %s", strerror(errno));
            env->FatalError("Exiting");
    }
}

static void sendTextEvent(JNIEnv *env, __unused jobject thiz, jlong ptr, jbyteArray text) {
    lastInputTimestampMs = nowMs();
    auto* r = (LorieViewResources*) ptr;
    if (r && r->connFd != -1 && text) {
        jsize length = env->GetArrayLength(text);
        jbyte *str = env->GetByteArrayElements(text, NULL);
        char *p = (char*) str;
        mbstate_t mbstate = { 0 };
        if (!length)
            return;

        log(DEBUG, "Parsing text: %.*s", length, str);

        while (*p) {
            wchar_t wc;
            size_t len = mbrtowc(&wc, p, MB_CUR_MAX, &mbstate);

            if (len == (size_t)-1 || len == (size_t)-2) {
                log(ERROR, "Invalid UTF-8 sequence encountered");
                break;
            }

            if (len == 0)
                break;

            log(DEBUG, "Sending unicode event: %lc (U+%X)", wc, wc);
            lorieEvent e = { .unicode = { .t = EVENT_UNICODE, .code = (uint32_t) wc } };
            write(r->connFd, &e, sizeof(e));
            p += len;
            if (p - (char*) str >= length)
                break;
            usleep(2500);
        }

        env->ReleaseByteArrayElements(text, str, JNI_ABORT);
    }
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, __unused void *reserved) {
    JNIEnv* env;
    static JNINativeMethod methods[] = {
            {"nativeInit", "()J", (void *)&nativeInit},
            {"nativeDestroy", "(J)V", (void *) +[](__unused JNIEnv *env, __unused jobject thiz, jlong ptr) {
                auto* r = (LorieViewResources*) ptr;
                if (!r) return;
                r->~LorieViewResources();
                free(r);
            }},
            {"surfaceChanged", "(JLandroid/view/Surface;)V", (void *) +[](JNIEnv *env, __unused jobject thiz, jlong ptr, jobject sfc) {
                auto* r = (LorieViewResources*) ptr;
                if (!r || r->destroyed) return;
                r->renderer.setWindow(env, sfc);
            }},
            {"setViewport", "(JIIIIIII)V", (void *) +[](__unused JNIEnv *env, __unused jobject thiz, jlong ptr, jint x, jint y, jint w, jint h, jint ew, jint eh, jint hidden) {
                auto* r = (LorieViewResources*) ptr;
                if (!r || r->destroyed) return;
                r->renderer.setViewport(x, y, w, h, ew, eh, hidden);
            }},
            {"setRendererZoom", "(JI)V", (void *) +[](__unused JNIEnv *env, __unused jobject thiz, jlong ptr, jint percent) {
                auto* r = (LorieViewResources*) ptr;
                if (!r || r->destroyed) return;
                r->renderer.setZoom(percent);
            }},
            {"setFiltering", "(JI)V", (void *) +[](__unused JNIEnv* env, __unused jobject self, jlong ptr, jint filtering) {
                auto* r = (LorieViewResources*) ptr;
                if (!r || r->destroyed) return;
                r->renderer.setFiltering(filtering);
            }},
            {"connect", "(JI)V", (void *) +[](__unused JNIEnv* env, __unused jclass clazz, jlong ptr, jint fd) {
                auto* r = (LorieViewResources*) ptr;
                if (!r) return;
                r->connect(fd);
            }},
            // @CriticalNative: no implicit JNIEnv*/jclass, unlike the @FastNative entries below.
            {"connected", "(J)Z", (void *) +[](jlong ptr) -> jboolean {
                auto* r = (LorieViewResources*) ptr;
                return r && r->connFd != -1;
            }},
            {"startLogcat", "(JI)V", (void *)&startLogcat},
            {"setClipboardSyncEnabled", "(JZZ)V", (void *) +[](__unused JNIEnv* env, __unused jobject cls, jlong ptr, jboolean enable, __unused jboolean ignored) {
                auto* r = (LorieViewResources*) ptr;
                sendEvent(r, .clipboardEnable = { .t = EVENT_CLIPBOARD_ENABLE, .enable = enable });
            }},
            {"sendClipboardAnnounce", "(J)V", (void *) +[](__unused JNIEnv *env, __unused jobject thiz, jlong ptr) {
                auto* r = (LorieViewResources*) ptr;
                sendEvent(r, .type = EVENT_CLIPBOARD_ANNOUNCE);
            }},
            {"sendClipboardEvent", "(J[B)V", (void *) +[](JNIEnv *env, __unused jobject thiz, jlong ptr, jbyteArray text) {
                auto* r = (LorieViewResources*) ptr;
                if (r && r->connFd != -1 && text) {
                    jsize length = env->GetArrayLength(text);
                    jbyte* str = env->GetByteArrayElements(text, NULL);
                    sendEvent(r, .clipboardSend = { .t = EVENT_CLIPBOARD_SEND, .count = (uint32_t) length });
                    write(r->connFd, str, length);
                    env->ReleaseByteArrayElements(text, str, JNI_ABORT);
                }
            }},
            {"sendWindowChange", "(JIIILjava/lang/String;)V", (void *) +[](__unused JNIEnv* env, __unused jobject cls, jlong ptr, jint width, jint height, jint framerate, jstring jname) {
                auto* r = (LorieViewResources*) ptr;
                if (r && r->connFd != -1) {
                    const char *name = (!jname || width <= 0 || height <= 0) ? NULL : env->GetStringUTFChars(jname, JNI_FALSE);
                    sendEvent(r, .screenSize = { .t = EVENT_SCREEN_SIZE, .width = (uint16_t) width, .height = (uint16_t) height, .framerate = (uint16_t) framerate, .name_size = (name ? strlen(name) : 0) });
                    if (name) {
                        write(r->connFd, name, strlen(name));
                        env->ReleaseStringUTFChars(jname, name);
                    }
                }
            }},
            {"sendMouseEvent", "(JFFIZZ)V", (void *) +[](JNIEnv* env, __unused jobject cls, jlong ptr, jfloat x, jfloat y, jint which_button, jboolean button_down, jboolean relative) {
                lastInputTimestampMs = nowMs();
                auto* r = (LorieViewResources*) ptr;
                if (r && r->connFd != -1) {
                    if (which_button > 0)
                        env->CallVoidMethod(r->thiz, MainActivity.resetIme);
                    sendEvent(r, .mouse = { .t = EVENT_MOUSE, .x = x, .y = y, .detail = (uint8_t) which_button, .down = button_down, .relative = relative });
                }
            }},
            {"sendTouchEvent", "(JIIII)V", (void *) +[](__unused JNIEnv* env, __unused jobject cls, jlong ptr, jint action, jint id, jint x, jint y) {
                lastInputTimestampMs = nowMs();
                auto* r = (LorieViewResources*) ptr;
                if (action != -1)
                    sendEvent(r, .touch = { .t = EVENT_TOUCH, .type = (uint16_t) action, .id = (uint16_t) id, .x = (uint16_t) x, .y = (uint16_t) y });
            }},
            {"sendStylusEvent", "(JFFIIIIIZZ)V", (void *) +[](JNIEnv *env, __unused jobject thiz, jlong ptr, jfloat x, jfloat y, jint pressure, jint tilt_x, jint tilt_y, jint orientation, jint buttons, jboolean eraser, jboolean mouse) {
                lastInputTimestampMs = nowMs();
                auto* r = (LorieViewResources*) ptr;
                if (r && r->connFd != -1) {
                    env->CallVoidMethod(r->thiz, MainActivity.resetIme);
                    sendEvent(r, .stylus = { .t = EVENT_STYLUS, .x = x, .y = y, .pressure = (uint16_t) pressure, .tilt_x = (int8_t) tilt_x, .tilt_y = (int8_t) tilt_y, .orientation = (int16_t) orientation, .buttons = (uint8_t) buttons, .eraser = eraser, .mouse = mouse });
                }
            }},
            {"requestStylusEnabled", "(JZ)V", (void *) +[](__unused JNIEnv *env, __unused jobject thiz, jlong ptr, jboolean enabled) {
                auto* r = (LorieViewResources*) ptr;
                sendEvent(r, .stylusEnable = { .t = EVENT_STYLUS_ENABLE, .enable = enabled });
            }},
            {"sendLockKeysState", "(JI)V", (void *) +[](__unused JNIEnv *env, __unused jobject thiz, jlong ptr, jint state) {
                auto* r = (LorieViewResources*) ptr;
                sendEvent(r, .lockKeysState = { .t = EVENT_LOCK_KEYS_STATE, .state = (uint8_t) state });
            }},
            {"sendKeyEvent", "(JIIZ)Z", (void *) +[](__unused JNIEnv* env, __unused jobject cls, jlong ptr, jint scan_code, jint key_code, jboolean key_down) -> jboolean {
                lastInputTimestampMs = nowMs();
                auto* r = (LorieViewResources*) ptr;
                if (r && r->connFd != -1) {
                    int code = (scan_code) ?: android_to_linux_keycode[key_code];
                    sendEvent(r, .key = { .t = EVENT_KEY, .key = (uint16_t) (code + 8), .state = key_down });
                }
                return true;
            }},
            {"sendTextEvent", "(J[B)V", (void *)&sendTextEvent},
            {"requestConnection", "(J)Z", (void *)&requestConnection},
            {"getLastInputTimestamp", "()J", (void *) +[](__unused JNIEnv* env, __unused jclass clazz) -> jlong {
                return (jlong) lastInputTimestampMs;
            }},
            {"markUserActivity", "()V", (void *) +[](__unused JNIEnv* env, __unused jclass clazz) {
                lastInputTimestampMs = nowMs();
            }},
    };
    vm->AttachCurrentThread(&env, NULL);
    jclass cls = env->FindClass("com/termux/x11/LorieView");
    env->RegisterNatives(cls, methods, sizeof(methods)/sizeof(methods[0]));

    return JNI_VERSION_1_6;
}


// It is needed to redirect stderr to logcat
static void* stderrToLogcatThread(__unused void* cookie) {
    FILE *fp;
    int p[2];
    size_t len;
    char *line = NULL;
    pipe(p);

    fp = fdopen(p[0], "r");

    dup2(p[1], 2);
    dup2(p[1], 1);
    while ((getline(&line, &len, fp)) != -1) {
        log(DEBUG, "%s%s", line, (line[len - 1] == '\n') ? "" : "\n");
    }

    return NULL;
}

extern char* __progname;
__attribute__((constructor)) static void init(void) {
    pthread_t t;
    if (!strcmp(__progname, "com.termux.x11"))
        pthread_create(&t, NULL, stderrToLogcatThread, NULL);
}
