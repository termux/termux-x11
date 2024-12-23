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
#include <wchar.h>
#include "lorie.h"

#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#define log(prio, ...) __android_log_print(ANDROID_LOG_ ## prio, "LorieNative", __VA_ARGS__)

extern char *__progname; // NOLINT(bugprone-reserved-identifier)
static int conn_fd = -1;

static struct {
    jclass self;
    jmethodID forName;
    jmethodID decode;
} Charset = {0};

static struct {
    jclass self;
    jmethodID toString;
} CharBuffer = {0};

static struct lorie_shared_server_state* state = NULL;
static AHardwareBuffer* sharedBuffer = NULL;

static jclass FindClassOrDie(JNIEnv *env, const char* name) {
    jclass clazz = (*env)->FindClass(env, name);
    if (!clazz) {
        char buffer[1024] = {0};
        sprintf(buffer, "class %s not found", name);
        log(ERROR, "%s", buffer);
        (*env)->FatalError(env, buffer);
        return NULL;
    }

    return (*env)->NewGlobalRef(env, clazz);
}

static jclass FindMethodOrDie(JNIEnv *env, jclass clazz, const char* name, const char* signature, jboolean isStatic) {
    __typeof__((*env)->GetMethodID) getMethodID = isStatic ? (*env)->GetStaticMethodID : (*env)->GetMethodID;
    jmethodID method = getMethodID(env, clazz, name, signature);
    if (!method) {
        char buffer[1024] = {0};
        sprintf(buffer, "method %s %s not found", name, signature);
        log(ERROR, "%s", buffer);
        (*env)->FatalError(env, buffer);
        return NULL;
    }

    return method;
}

static inline void checkConnection(JNIEnv* env) {
    int retval, b = 0;

    if (conn_fd == -1)
        return;

    if ((retval = recv(conn_fd, &b, 1, MSG_PEEK)) <= 0 && errno != EAGAIN) {
        log(DEBUG, "recv %d %s", retval, strerror(errno));
        jclass cls = (*env)->FindClass(env, "com/termux/x11/CmdEntryPoint");
        jmethodID method = !cls ? NULL : (*env)->GetStaticMethodID(env, cls, "requestConnection", "()V");
        if (method)
            (*env)->CallStaticVoidMethod(env, cls, method);

        close(conn_fd);
        conn_fd = -1;
    }
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_connect(__unused JNIEnv* env, __unused jobject cls, jint fd) {
    if (!Charset.self) {
        // Init clipboard-related JNI stuff
        Charset.self = FindClassOrDie(env, "java/nio/charset/Charset");
        Charset.forName = FindMethodOrDie(env, Charset.self, "forName", "(Ljava/lang/String;)Ljava/nio/charset/Charset;", JNI_TRUE);
        Charset.decode = FindMethodOrDie(env, Charset.self, "decode", "(Ljava/nio/ByteBuffer;)Ljava/nio/CharBuffer;", JNI_FALSE);

        CharBuffer.self = FindClassOrDie(env,  "java/nio/CharBuffer");
        CharBuffer.toString = FindMethodOrDie(env, CharBuffer.self, "toString", "()Ljava/lang/String;", JNI_FALSE);
    }

    conn_fd = fd;
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    checkConnection(env);
    log(DEBUG, "XCB connection is successfull");
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_handleXEvents(JNIEnv *env, jobject thiz) {
    checkConnection(env);
    if (conn_fd != -1) {
        lorieEvent e = {0};

        again:
        if (read(conn_fd, &e, sizeof(e)) == sizeof(e)) {
            switch(e.type) {
                case EVENT_CLIPBOARD_SEND: {
                    if (!e.clipboardSend.count)
                        break;
                    char clipboard[e.clipboardSend.count + 1];
                    memset(clipboard, 0, e.clipboardSend.count + 1);
                    read(conn_fd, clipboard, sizeof(clipboard));
                    clipboard[e.clipboardSend.count] = 0;
                    log(DEBUG, "Clipboard content (%zu symbols) is %s", strlen(clipboard), clipboard);
                    jmethodID id = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, thiz), "setClipboardText","(Ljava/lang/String;)V");
                    jobject bb = (*env)->NewDirectByteBuffer(env, clipboard, strlen(clipboard));
                    jobject charset = (*env)->CallStaticObjectMethod(env, Charset.self, Charset.forName, (*env)->NewStringUTF(env, "UTF-8"));
                    jobject cb = (*env)->CallObjectMethod(env, charset, Charset.decode, bb);
                    (*env)->DeleteLocalRef(env, bb);

                    jstring str = (*env)->CallObjectMethod(env, cb, CharBuffer.toString);
                    (*env)->CallVoidMethod(env, thiz, id, str);
                    break;
                }
                case EVENT_CLIPBOARD_REQUEST: {
                    (*env)->CallVoidMethod(env, thiz, (*env)->GetMethodID(env, (*env)->GetObjectClass(env, thiz), "requestClipboard", "()V"));
                    break;
                }
                case EVENT_SHARED_SERVER_STATE: {
                    int fd = ancil_recv_fd(conn_fd);

                    if (!(state = mmap(NULL, sizeof(*state), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)))
                        log(ERROR, "Failed to map server state.");

                    close(fd); // Closing file descriptor does not unmmap shared memory fragment.
                    break;
                }
                case EVENT_SHARED_ROOT_WINDOW_BUFFER: {
                    if (sharedBuffer)
                        AHardwareBuffer_release(sharedBuffer);
                    int error = AHardwareBuffer_recvHandleFromUnixSocket(conn_fd, &sharedBuffer);
                    AHardwareBuffer_Desc desc = {0};
                    if (sharedBuffer)
                        AHardwareBuffer_describe(sharedBuffer, &desc);
                    log(INFO, "Received shared buffer width %d height %d format %d", desc.width, desc.height, desc.format);
                }
                case EVENT_REQUEST_RENDER: {
                    // Currently not implemented, renderer is still running in X server process.
                }
            }
        }

        int n;
        if (ioctl(conn_fd, FIONREAD, &n) >= 0 && n > sizeof(e))
            goto again;
    }
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_startLogcat(JNIEnv *env, __unused jobject cls, jint fd) {
    log(DEBUG, "Starting logcat with output to given fd");

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
            (*env)->FatalError(env, "Exiting");
    }
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_setClipboardSyncEnabled(__unused JNIEnv* env, __unused jobject cls, jboolean enable, __unused jboolean ignored) {
    if (conn_fd != -1) {
        lorieEvent e = { .clipboardEnable = { .t = EVENT_CLIPBOARD_ENABLE, .enable = enable } };
        write(conn_fd, &e, sizeof(e));
        checkConnection(env);
    }
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_sendClipboardAnnounce(JNIEnv *env, __unused jobject thiz) {
    if (conn_fd != -1) {
        lorieEvent e = { .type = EVENT_CLIPBOARD_ANNOUNCE };
        write(conn_fd, &e, sizeof(e));
        checkConnection(env);
    }
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_sendClipboardEvent(JNIEnv *env, __unused jobject thiz, jbyteArray text) {
    if (conn_fd != -1 && text) {
        jsize length = (*env)->GetArrayLength(env, text);
        jbyte* str = (*env)->GetByteArrayElements(env, text, NULL);
        lorieEvent e = { .clipboardSend = { .t = EVENT_CLIPBOARD_SEND, .count = length } };
        write(conn_fd, &e, sizeof(e));
        write(conn_fd, str, length);
        (*env)->ReleaseByteArrayElements(env, text, str, JNI_ABORT);
        checkConnection(env);
    }
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_sendWindowChange(__unused JNIEnv* env, __unused jobject cls, jint width, jint height, jint framerate, jstring jname) {
    if (conn_fd != -1) {
        const char *name = (!jname || width <= 0 || height <= 0) ? NULL : (*env)->GetStringUTFChars(env, jname, JNI_FALSE);
        lorieEvent e = { .screenSize = { .t = EVENT_SCREEN_SIZE, .width = width, .height = height, .framerate = framerate, .name_size = (name ? strlen(name) : 0) } };
        write(conn_fd, &e, sizeof(e));
        if (name) {
            write(conn_fd, name, strlen(name));
            (*env)->ReleaseStringUTFChars(env, jname, name);
        }
        checkConnection(env);
    }
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_sendMouseEvent(__unused JNIEnv* env, __unused jobject cls, jfloat x, jfloat y, jint which_button, jboolean button_down, jboolean relative) {
    if (conn_fd != -1) {
        lorieEvent e = { .mouse = { .t = EVENT_MOUSE, .x = x, .y = y, .detail = which_button, .down = button_down, .relative = relative } };
        write(conn_fd, &e, sizeof(e));
        checkConnection(env);
    }
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_sendTouchEvent(__unused JNIEnv* env, __unused jobject cls, jint action, jint id, jint x, jint y) {
    if (conn_fd != -1 && action != -1) {
        lorieEvent e = { .touch = { .t = EVENT_TOUCH, .type = action, .id = id, .x = x, .y = y } };
        write(conn_fd, &e, sizeof(e));
        checkConnection(env);
    }
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_sendStylusEvent(JNIEnv *env, __unused jobject thiz, jfloat x, jfloat y,
                                              jint pressure, jint tilt_x, jint tilt_y,
                                              jint orientation, jint buttons, jboolean eraser, jboolean mouse) {
    if (conn_fd != -1) {
        lorieEvent e = { .stylus = { .t = EVENT_STYLUS, .x = x, .y = y, .pressure = pressure, .tilt_x = tilt_x, .tilt_y = tilt_y, .orientation = orientation, .buttons = buttons, .eraser = eraser, .mouse = mouse } };
        write(conn_fd, &e, sizeof(e));
        checkConnection(env);
    }
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_requestStylusEnabled(JNIEnv *env, __unused jclass clazz, jboolean enabled) {
    if (conn_fd != -1) {
        lorieEvent e = { .stylusEnable = { .t = EVENT_STYLUS_ENABLE, .enable = enabled } };
        write(conn_fd, &e, sizeof(e));
        checkConnection(env);
    }
}

JNIEXPORT jboolean JNICALL
Java_com_termux_x11_LorieView_sendKeyEvent(__unused JNIEnv* env, __unused jobject cls, jint scan_code, jint key_code, jboolean key_down) {
    if (conn_fd != -1) {
        int code = (scan_code) ?: android_to_linux_keycode[key_code];
        log(DEBUG, "Sending key: %d (%d %d %d)", code + 8, scan_code, key_code, key_down);
        lorieEvent e = { .key = { .t = EVENT_KEY, .key = code + 8, .state = key_down } };
        write(conn_fd, &e, sizeof(e));
        checkConnection(env);
    }

    return true;
}

JNIEXPORT void JNICALL
Java_com_termux_x11_LorieView_sendTextEvent(JNIEnv *env, __unused jobject thiz, jbyteArray text) {
    if (conn_fd != -1 && text) {
        jsize length = (*env)->GetArrayLength(env, text);
        jbyte *str = (*env)->GetByteArrayElements(env, text, NULL);
        char *p = (char*) str;
        mbstate_t state = { 0 };
        if (!length)
            return;

        log(DEBUG, "Parsing text: %.*s", length, str);

        while (*p) {
            wchar_t wc;
            size_t len = mbrtowc(&wc, p, MB_CUR_MAX, &state);

            if (len == (size_t)-1 || len == (size_t)-2) {
                log(ERROR, "Invalid UTF-8 sequence encountered");
                break;
            }

            if (len == 0)
                break;

            log(DEBUG, "Sending unicode event: %lc (U+%X)", wc, wc);
            lorieEvent e = { .unicode = { .t = EVENT_UNICODE, .code = wc } };
            write(conn_fd, &e, sizeof(e));
            p += len;
            if (p - (char*) str >= length)
                break;
            usleep(30000);
        }

        (*env)->ReleaseByteArrayElements(env, text, str, JNI_ABORT);
        checkConnection(env);
    }
}

#if 1
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

__attribute__((constructor)) static void init(void) {
    pthread_t t;
    pthread_create(&t, NULL, stderrToLogcatThread, NULL);
}
#endif
