#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
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
#include "lorie.h"

#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#pragma ide diagnostic ignored "ConstantFunctionResult"
#define log(prio, ...) __android_log_print(ANDROID_LOG_ ## prio, "LorieNative", __VA_ARGS__)
#define sendEvent(...) do { if (conn_fd != -1) { lorieEvent e = { __VA_ARGS__ }; write(conn_fd, &e, sizeof(e)); } } while (0)

extern volatile int conn_fd; // The only variable from shared with X server code.
bool lorieDebugEnabled = false;

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

static JNIEnv *guienv = NULL; // Must be used only in GUI thread.
static jobject globalThiz = NULL;
static Renderer g_renderer;

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

static jboolean requestConnection(__unused JNIEnv *env, __unused jclass clazz) {
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

static void connect_(__unused JNIEnv* env, __unused jobject cls, jint fd);
static void nativeInit(JNIEnv *env, jobject thiz) {
    JavaVM* vm;
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

    g_renderer.init(env);

    env->GetJavaVM(&vm);
    vm->AttachCurrentThread(&guienv, NULL);
    globalThiz = guienv->NewGlobalRef(thiz);
    connect_(NULL, NULL, -1);
}

static int xcallback(int fd, int events, __unused void* data) {
    JNIEnv *env = guienv;
    jobject thiz = globalThiz;

    if (events & (ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP)) {
        jobject instance = env->CallStaticObjectMethod(MainActivity.self, MainActivity.getInstance);
        if (instance)
            env->CallVoidMethod(instance, MainActivity.clientConnectedStateChanged);

        ALooper_removeFd(ALooper_forThread(), fd);
        close(conn_fd);
        conn_fd = -1;
        g_renderer.setSharedState(NULL);
        g_renderer.removeAllBuffers();
        log(DEBUG, "disconnected");
        return 1;
    }

    if (conn_fd != -1) {
        lorieEvent e = {0};

        again:
        if (read(conn_fd, &e, sizeof(e)) == sizeof(e)) {
            switch(e.type) {
                case EVENT_CLIPBOARD_SEND: {
                    char clipboard[e.clipboardSend.count + 1];
                    memset(clipboard, 0, sizeof(clipboard));
                    read(conn_fd, clipboard, e.clipboardSend.count);
                    log(DEBUG, "Got clipboard text from X server (%u bytes)", e.clipboardSend.count);

                    jmethodID id = env->GetMethodID(env->GetObjectClass(thiz), "setClipboardText","(Ljava/lang/String;)V");
                    jobject bb = env->NewDirectByteBuffer(clipboard, e.clipboardSend.count);
                    jobject charset = env->CallStaticObjectMethod(Charset.self, Charset.forName, env->NewStringUTF("UTF-8"));
                    jobject cb = env->CallObjectMethod(charset, Charset.decode, bb);
                    env->DeleteLocalRef(bb);

                    jstring str = (jstring) env->CallObjectMethod(cb, CharBuffer.toString);
                    env->CallVoidMethod(thiz, id, str);
                    break;
                }
                case EVENT_CLIPBOARD_SEND_IMAGE: {
                    int imgFd = ancil_recv_fd(conn_fd);
                    if (imgFd < 0) {
                        log(ERROR, "Failed to receive clipboard image fd from X server");
                        break;
                    }

                    log(DEBUG, "Got clipboard image from X server (%s, %u bytes, fd=%d)", e.clipboardSendImage.mime, e.clipboardSendImage.size, imgFd);
                    jmethodID id = env->GetMethodID(env->GetObjectClass(thiz), "setClipboardImage", "(Ljava/lang/String;I)V");
                    env->CallVoidMethod(thiz, id, env->NewStringUTF(e.clipboardSendImage.mime), imgFd); // ownership of the fd passes to Java
                    break;
                }
                case EVENT_CLIPBOARD_REQUEST: {
                    env->CallVoidMethod(thiz, env->GetMethodID(env->GetObjectClass(thiz), "requestClipboard", "()V"));
                    break;
                }
                case EVENT_SHARED_SERVER_STATE: {
                    struct lorie_shared_server_state* state = NULL;
                    int stateFd = ancil_recv_fd(conn_fd);

                    if (stateFd < 0)
                        break;

                    state = (struct lorie_shared_server_state*) mmap(NULL, sizeof(*state), PROT_READ|PROT_WRITE, MAP_SHARED, stateFd, 0);
                    if (!state || state == MAP_FAILED) {
                        log(ERROR, "Failed to map server state: %s", strerror(errno));
                        state = NULL;
                    }

                    g_renderer.setSharedState(state);

                    close(stateFd); // Closing file descriptor does not unmmap shared memory fragment.
                    break;
                }
                case EVENT_ADD_BUFFER: {
                    static LorieBuffer* buffer = NULL;
                    const LorieBuffer_Desc* desc;
                    LorieBuffer_recvHandleFromUnixSocket(conn_fd, &buffer);
                    desc = LorieBuffer_description(buffer);
                    log(INFO, "Received shared buffer width %d stride %d height %d format %d type %d id %llu", desc->width, desc->stride, desc->height, desc->format, desc->type, desc->id);
                    g_renderer.addBuffer(buffer);
                    break;
                }
                case EVENT_REMOVE_BUFFER: {
                    g_renderer.removeBuffer(e.removeBuffer.id);
                    break;
                }
                case EVENT_WINDOW_FOCUS_CHANGED: {
                    env->CallVoidMethod(thiz, MainActivity.resetIme);
                }
            }
        }

        int n;
        if (ioctl(conn_fd, FIONREAD, &n) >= 0 && n > sizeof(e))
            goto again;
    }

    return 1;
}

static void connect_(__unused JNIEnv* env, __unused jobject cls, jint fd) {
    if (conn_fd != -1) {
        ALooper_removeFd(ALooper_forThread(), conn_fd);
        close(conn_fd);
        g_renderer.setSharedState(NULL);
        g_renderer.removeAllBuffers();
        log(DEBUG, "disconnected");
    }

    if ((conn_fd = fd) != -1) {
        ALooper_addFd(ALooper_forThread(), fd, 0, ALOOPER_EVENT_INPUT | ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP, xcallback, NULL);

        // Give the X server our renderer wakeup cond var fd, resent on every reconnect.
        lorieEvent e = { .type = EVENT_RENDERER_WAKEUP_COND };
        write(conn_fd, &e, sizeof(e));
        ancil_send_fd(conn_fd, g_renderer.getWakeupCondFd());

        log(DEBUG, "XCB connection is successfull");
    }
}

static void startLogcat(JNIEnv *env, __unused jobject cls, jint fd) {
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

static void sendTextEvent(JNIEnv *env, __unused jobject thiz, jbyteArray text) {
    if (conn_fd != -1 && text) {
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
            write(conn_fd, &e, sizeof(e));
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
            {"nativeInit", "()V", (void *)&nativeInit},
            {"surfaceChanged", "(Landroid/view/Surface;)V", (void *) +[](JNIEnv *env, __unused jobject thiz, jobject sfc) {
                g_renderer.setWindow(env, sfc);
            }},
            {"setViewport", "(IIIIIII)V", (void *) +[](__unused JNIEnv *env, __unused jclass clazz, jint x, jint y, jint w, jint h, jint ew, jint eh, jint hidden) {
                g_renderer.setViewport(x, y, w, h, ew, eh, hidden);
            }},
            {"setRendererZoom", "(I)V", (void *) +[](__unused JNIEnv *env, __unused jclass clazz, jint percent) {
                g_renderer.setZoom(percent);
            }},
            {"setFiltering", "(I)V", (void *) +[](__unused JNIEnv* env, __unused jobject self, jint filtering) {
                g_renderer.setFiltering(filtering);
            }},
            {"connect", "(I)V", (void *)&connect_},
            {"connected", "()Z", (void *) +[](__unused JNIEnv* env, __unused jclass clazz) -> jboolean {
                return conn_fd != -1;
            }},
            {"startLogcat", "(I)V", (void *)&startLogcat},
            {"setClipboardSyncEnabled", "(ZZ)V", (void *) +[](__unused JNIEnv* env, __unused jobject cls, jboolean enable, __unused jboolean ignored) {
                sendEvent(.clipboardEnable = { .t = EVENT_CLIPBOARD_ENABLE, .enable = enable });
            }},
            {"sendClipboardAnnounce", "()V", (void *) +[](__unused JNIEnv *env, __unused jobject thiz) {
                sendEvent(.type = EVENT_CLIPBOARD_ANNOUNCE);
            }},
            {"sendClipboardEvent", "(Ljava/lang/String;[B)V", (void *) +[](JNIEnv *env, __unused jobject thiz, jstring mime, jbyteArray data) {
                if (conn_fd == -1)
                    return;

                const char* mimeStr = env->GetStringUTFChars(mime, NULL);
                size_t dataLen = data ? (size_t) env->GetArrayLength(data) : 0;
                log(DEBUG, "Sending clipboard data to X server (%s, %zu bytes)", mimeStr, dataLen);

                if (!strncmp(mimeStr, "image/", 6) && dataLen) {
                    int fd = LorieBuffer_createRegion("x11-clipboard-image", dataLen);
                    if (fd < 0) {
                        log(ERROR, "Failed to create shared memory region for clipboard image: %s", strerror(errno));
                    } else {
                        void* mem = mmap(NULL, dataLen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                        if (mem == MAP_FAILED) {
                            log(ERROR, "Failed to mmap clipboard image region: %s", strerror(errno));
                            close(fd);
                        } else {
                            jbyte* bytes = env->GetByteArrayElements(data, NULL);
                            memcpy(mem, bytes, dataLen);
                            env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
                            munmap(mem, dataLen);

                            lorieEvent e = { .clipboardSendImage = { .t = EVENT_CLIPBOARD_SEND_IMAGE, .size = (uint32_t) dataLen } };
                            snprintf(e.clipboardSendImage.mime, sizeof(e.clipboardSendImage.mime), "%s", mimeStr);
                            write(conn_fd, &e, sizeof(e));
                            ancil_send_fd(conn_fd, fd);
                            close(fd);
                        }
                    }
                } else {
                    lorieEvent e = { .clipboardSend = { .t = EVENT_CLIPBOARD_SEND, .count = (uint32_t) dataLen } };
                    write(conn_fd, &e, sizeof(e));
                    if (dataLen) {
                        jbyte* bytes = env->GetByteArrayElements(data, NULL);
                        write(conn_fd, bytes, dataLen);
                        env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
                    }
                }

                env->ReleaseStringUTFChars(mime, mimeStr);
            }},
            {"sendWindowChange", "(IIILjava/lang/String;)V", (void *) +[](__unused JNIEnv* env, __unused jobject cls, jint width, jint height, jint framerate, jstring jname) {
                if (conn_fd != -1) {
                    const char *name = (!jname || width <= 0 || height <= 0) ? NULL : env->GetStringUTFChars(jname, JNI_FALSE);
                    sendEvent(.screenSize = { .t = EVENT_SCREEN_SIZE, .width = (uint16_t) width, .height = (uint16_t) height, .framerate = (uint16_t) framerate, .name_size = (name ? strlen(name) : 0) });
                    if (name) {
                        write(conn_fd, name, strlen(name));
                        env->ReleaseStringUTFChars(jname, name);
                    }
                }
            }},
            {"sendMouseEvent", "(FFIZZ)V", (void *) +[](__unused JNIEnv* env, __unused jobject cls, jfloat x, jfloat y, jint which_button, jboolean button_down, jboolean relative) {
                if (conn_fd != -1) {
                    if (which_button > 0)
                        env->CallVoidMethod(globalThiz, MainActivity.resetIme);
                    sendEvent(.mouse = { .t = EVENT_MOUSE, .x = x, .y = y, .detail = (uint8_t) which_button, .down = button_down, .relative = relative });
                }
            }},
            {"sendTouchEvent", "(IIII)V", (void *) +[](__unused JNIEnv* env, __unused jobject cls, jint action, jint id, jint x, jint y) {
                if (action != -1)
                    sendEvent(.touch = { .t = EVENT_TOUCH, .type = (uint16_t) action, .id = (uint16_t) id, .x = (uint16_t) x, .y = (uint16_t) y });
            }},
            {"sendStylusEvent", "(FFIIIIIZZ)V", (void *) +[](__unused JNIEnv *env, __unused jobject thiz, jfloat x, jfloat y, jint pressure, jint tilt_x, jint tilt_y, jint orientation, jint buttons, jboolean eraser, jboolean mouse) {
                if (conn_fd != -1) {
                    env->CallVoidMethod(globalThiz, MainActivity.resetIme);
                    sendEvent(.stylus = { .t = EVENT_STYLUS, .x = x, .y = y, .pressure = (uint16_t) pressure, .tilt_x = (int8_t) tilt_x, .tilt_y = (int8_t) tilt_y, .orientation = (int16_t) orientation, .buttons = (uint8_t) buttons, .eraser = eraser, .mouse = mouse });
                }
            }},
            {"requestStylusEnabled", "(Z)V", (void *) +[](__unused JNIEnv *env, __unused jclass clazz, jboolean enabled) {
                sendEvent(.stylusEnable = { .t = EVENT_STYLUS_ENABLE, .enable = enabled });
            }},
            {"sendKeyEvent", "(IIZ)Z", (void *) +[](__unused JNIEnv* env, __unused jobject cls, jint scan_code, jint key_code, jboolean key_down) -> jboolean {
                if (conn_fd != -1) {
                    int code = (scan_code) ?: android_to_linux_keycode[key_code];
                    sendEvent(.key = { .t = EVENT_KEY, .key = (uint16_t) (code + 8), .state = key_down });
                }
                return true;
            }},
            {"sendTextEvent", "([B)V", (void *)&sendTextEvent},
            {"requestConnection", "()Z", (void *)&requestConnection},
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
