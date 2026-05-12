#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "DanglingPointer"
#pragma ide diagnostic ignored "ConstantConditionsOC"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "UnreachableCode"
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#pragma ide diagnostic ignored "misc-no-recursion"
#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <media/NdkImageReader.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include "list.h"
#include "lorie.h"

#define log(...) __android_log_print(ANDROID_LOG_DEBUG, "gles-renderer", __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, "gles-renderer", __VA_ARGS__)

static GLuint createProgram(const char* p_vertex_source, const char* p_fragment_source);

static int printEglError(char* msg, int line) {
    char descBuf[32] = {0};
    char* desc;
    int err = eglGetError();
    switch(err) {
#define E(code, text) case code: desc = (char*) text; break
        case EGL_SUCCESS: desc = NULL; // "No error"
        E(EGL_NOT_INITIALIZED, "EGL not initialized or failed to initialize");
        E(EGL_BAD_ACCESS, "Resource inaccessible");
        E(EGL_BAD_ALLOC, "Cannot allocate resources");
        E(EGL_BAD_ATTRIBUTE, "Unrecognized attribute or attribute value");
        E(EGL_BAD_CONTEXT, "Invalid EGL context");
        E(EGL_BAD_CONFIG, "Invalid EGL frame buffer configuration");
        E(EGL_BAD_CURRENT_SURFACE, "Current surface is no longer valid");
        E(EGL_BAD_DISPLAY, "Invalid EGL display");
        E(EGL_BAD_SURFACE, "Invalid surface");
        E(EGL_BAD_MATCH, "Inconsistent arguments");
        E(EGL_BAD_PARAMETER, "Invalid argument");
        E(EGL_BAD_NATIVE_PIXMAP, "Invalid native pixmap");
        E(EGL_BAD_NATIVE_WINDOW, "Invalid native window");
        E(EGL_CONTEXT_LOST, "Context lost");
#undef E
        default:
            snprintf(descBuf, sizeof(descBuf) - 1, "Unknown error (%d)", err);
            desc = descBuf;
    }

    if (desc)
        log("renderer: %s: %s (%s:%d)\n", msg, desc, __FILE__, line);

    return 0;
}

static inline __always_inline void vprintEglError(char* msg, int line) {
    printEglError(msg, line);
}

static void checkGlError(int line) {
    GLenum error;
    char *desc = NULL;
    for (error = glGetError(); error; error = glGetError()) {
        switch (error) {
#define E(code) case code: desc = (char*)#code; break
            E(GL_INVALID_ENUM);
            E(GL_INVALID_VALUE);
            E(GL_INVALID_OPERATION);
            E(GL_STACK_OVERFLOW_KHR);
            E(GL_STACK_UNDERFLOW_KHR);
            E(GL_OUT_OF_MEMORY);
            E(GL_INVALID_FRAMEBUFFER_OPERATION);
            E(GL_CONTEXT_LOST_KHR);
            default:
                continue;
#undef E
        }
        log("Xlorie: GLES %d ERROR: %s.\n", line, desc);
        return;
    }
}

#define checkGlError() checkGlError(__LINE__)

static const char vertexShaderSrc[] =
    "attribute vec4 position;\n"
    "attribute vec2 texCoords;"
    "varying vec2 outTexCoords;\n"
    "void main(void) {\n"
    "   outTexCoords = texCoords;\n"
    "   gl_Position = position;\n"
    "}\n";

#define FRAGMENT_SHADER(texture) \
    "precision mediump float;\n" \
    "varying vec2 outTexCoords;\n" \
    "uniform sampler2D texture;\n" \
    "void main(void) {\n" \
    "   gl_FragColor = texture2D(texture, outTexCoords)" texture ";\n" \
    "}\n"

static const char fragmentShaderSrc[] = FRAGMENT_SHADER();
static const char fragmentShaderBgraSrc[] = FRAGMENT_SHADER(".bgra");

static EGLDisplay egl_display = EGL_NO_DISPLAY;
static EGLContext ctx = EGL_NO_CONTEXT;
static EGLSurface defaultSfc = EGL_NO_SURFACE, sfc = EGL_NO_SURFACE;
static EGLConfig cfg = 0;
static ANativeWindow *defaultWin = NULL, *win = NULL;
static volatile struct xorg_list addedBuffers, buffers, removedBuffers;
volatile jint filtering = GL_NEAREST;

static JNIEnv* renderEnv = NULL;
static JavaVM* rendererVm = NULL;
static jclass displayLinkBridgeClass = NULL;
static jmethodID displayLinkPushFrameMethod = NULL;
static jmethodID displayLinkCaptureReadyMethod = NULL;
static jmethodID displayLinkCaptureWidthMethod = NULL;
static jmethodID displayLinkCaptureHeightMethod = NULL;
static GLuint displaylinkSinkFramebuffer = 0;
static GLuint displaylinkSinkFramebufferTexture = 0;
static int displaylinkSinkFramebufferWidth = 0;
static int displaylinkSinkFramebufferHeight = 0;
static volatile bool stateChanged = false, windowChanged = false;
static volatile struct lorie_shared_server_state* pendingState = NULL;
static volatile ANativeWindow* pendingWin = NULL;

static pthread_mutex_t stateLock;
static pthread_cond_t stateCond;
static pthread_cond_t stateChangeFinishCond;
static pthread_spinlock_t bufferLock;
static volatile struct lorie_shared_server_state* state = NULL;
static struct {
    GLuint id;
    bool cursorChanged;
} cursor;

static volatile bool displaylinkSinkChecked = false;
static volatile bool displaylinkSinkEnabledCached = false;
static int displaylinkSinkFrameCounter = 0;
static int displaylinkSinkAttemptCounter = 0;
static const int displaylinkSinkMaxFrames = 0;
static volatile bool displaylinkSinkLogged = false;
static char displaylinkSinkOutputDir[PATH_MAX] = {0};
static int displaylinkSinkCheckCount = 0;
static int displaylinkSinkProbeCount = 0;
static volatile bool displaylinkSinkCapturePending = false;
static volatile bool displaylinkSinkCaptureInProgress = false;
static int displaylinkSinkCaptureWidth = 0;
static int displaylinkSinkCaptureHeight = 0;
static int displaylinkSinkJavaDiagCount = 0;
static uint8_t* displaylinkSinkPostBuffer = NULL;
static size_t displaylinkSinkPostBufferSize = 0;

GLuint g_texture_program = 0, gv_pos = 0, gv_coords = 0;
GLuint g_texture_program_bgra = 0, gv_pos_bgra = 0, gv_coords_bgra = 0;

static void* rendererThread(void);
static void draw(GLuint id, float x0, float y0, float x1, float y1, float xfactor, uint8_t flip);
static void drawCursor(float displayWidth, float displayHeight);
static void displaylinkSinkDumpFrame(const uint8_t* pixels, int width, int height);
static bool displaylinkSinkMarkerExists(const char* path);
static bool displaylinkSinkDirWritable(const char* path);
static const char* displaylinkSinkResolveOutputDir(void);
static void displaylinkSinkWriteAliveMarker(void);
static void displaylinkSinkWriteJavaDiag(const char* message);
static void displaylinkSinkDumpCurrentFramebuffer(int width, int height);
static bool displaylinkSinkPostCurrentFramebuffer(int width, int height);
static bool displaylinkSinkPostFrameToJava(const uint8_t* pixels, int width, int height, int rowStride);
static bool displaylinkSinkEnsureFramebuffer(int width, int height);
static bool displaylinkSinkRenderAndPostFramebuffer(LorieBuffer* buffer, const LorieBuffer_Desc* desc, float xfactor, int width, int height);
static void displaylinkSinkDrawCursorLetterboxed(int sourceWidth, int sourceHeight, int targetWidth, int targetHeight);

static void pthreadCondVarProxyInit(void);
static void* pthreadCondVarProxyThread(void* cookie);
static void pthreadCondVarProxyListenOtherCondVar(pthread_cond_t* var);

static bool displaylinkSinkMarkerExists(const char* path) {
    return path && access(path, F_OK) == 0;
}

static bool displaylinkSinkDirWritable(const char* path) {
    return path && access(path, W_OK | X_OK) == 0;
}

static const char* displaylinkSinkResolveOutputDir(void) {
    const char* tmpdir = getenv("TMPDIR");
    const char* candidates[] = {
        tmpdir,
        "/sdcard/Download",
        "/data/local/tmp",
        "/tmp",
        NULL,
    };

    if (displaylinkSinkOutputDir[0])
        return displaylinkSinkOutputDir;

    for (int i = 0; candidates[i]; ++i) {
        if (displaylinkSinkDirWritable(candidates[i])) {
            snprintf(displaylinkSinkOutputDir, sizeof(displaylinkSinkOutputDir), "%s", candidates[i]);
            return displaylinkSinkOutputDir;
        }
    }

    snprintf(displaylinkSinkOutputDir, sizeof(displaylinkSinkOutputDir), "%s", "/tmp");
    return displaylinkSinkOutputDir;
}

static void displaylinkSinkWriteAliveMarker(void) {
    const char* outDir = displaylinkSinkResolveOutputDir();
    char path[PATH_MAX];
    int fd;

    if (!outDir)
        return;

    snprintf(path, sizeof(path), "%s/termux-x11-sink-alive.txt", outDir);
    fd = open(path, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, 0644);
    if (fd < 0)
        return;

    dprintf(fd, "sink alive pid=%d\n", getpid());
    close(fd);
}

static void displaylinkSinkWriteJavaDiag(const char* message) {
    const char* outDir = displaylinkSinkResolveOutputDir();
    char path[PATH_MAX];
    int fd;

    if (!outDir || !message)
        return;

    snprintf(path, sizeof(path), "%s/displaylink-java-diag.log", outDir);
    fd = open(path, O_CREAT | O_APPEND | O_WRONLY | O_CLOEXEC, 0644);
    if (fd < 0)
        return;

    dprintf(fd, "%ld %s\n", (long) time(NULL), message);
    close(fd);
}

bool displaylinkSinkEnabled(void) {
    if (!displaylinkSinkChecked) {
        const char* enabled = getenv("TERMUX_X11_DISPLAYLINK_SINK");
        const char* tmpdir = getenv("TMPDIR");
        char markerPath[PATH_MAX] = {0};
        const char* fallbackMarker = "/sdcard/Download/.termux-x11-displaylink-sink";
        const char* fallbackMarkerAlt = "/data/local/tmp/.termux-x11-displaylink-sink";

        if (tmpdir && snprintf(markerPath, sizeof(markerPath), "%s/.termux-x11-displaylink-sink", tmpdir) >= (int) sizeof(markerPath))
            markerPath[0] = '\0';

        ++displaylinkSinkCheckCount;
        log("DisplayLink sink check #%d: env=%s tmpdir=%s marker=%s fallback=%s alt=%s",
            displaylinkSinkCheckCount,
            enabled ?: "(null)",
            tmpdir ?: "(null)",
            markerPath[0] ? markerPath : "(null)",
            fallbackMarker,
            fallbackMarkerAlt);

        displaylinkSinkEnabledCached =
            (enabled && strcmp(enabled, "1") == 0) ||
            displaylinkSinkMarkerExists(markerPath[0] ? markerPath : NULL) ||
            displaylinkSinkMarkerExists(fallbackMarker) ||
            displaylinkSinkMarkerExists(fallbackMarkerAlt);
        displaylinkSinkChecked = true;
        if (displaylinkSinkEnabledCached && !displaylinkSinkLogged) {
            if (enabled && strcmp(enabled, "1") == 0)
                log("DisplayLink sink enabled via TERMUX_X11_DISPLAYLINK_SINK=1.");
            else if (displaylinkSinkMarkerExists(markerPath[0] ? markerPath : NULL))
                log("DisplayLink sink enabled via marker file %s.", markerPath);
            else if (displaylinkSinkMarkerExists(fallbackMarker))
                log("DisplayLink sink enabled via marker file %s.", fallbackMarker);
            else
                log("DisplayLink sink enabled via marker file %s.", fallbackMarkerAlt);
            log("Frames will be dumped to %s for integration testing.", displaylinkSinkResolveOutputDir());
            displaylinkSinkWriteAliveMarker();
            displaylinkSinkLogged = true;
        } else {
            log("DisplayLink sink disabled after marker/env check.");
        }
    }

    return displaylinkSinkEnabledCached;
}

void rendererSetJavaVm(JavaVM* vm) {
    rendererVm = vm;
    displaylinkSinkWriteJavaDiag(vm ? "rendererSetJavaVm: vm ready" : "rendererSetJavaVm: vm null");
}

void rendererInitDisplayLinkBridge(JNIEnv* env) {
    jclass localClass;

    if (displayLinkBridgeClass && displayLinkPushFrameMethod && displayLinkCaptureReadyMethod &&
        displayLinkCaptureWidthMethod && displayLinkCaptureHeightMethod)
        return;

    localClass = (*env)->FindClass(env, "com/termux/x11/displaylink/DisplayLinkBridge");
    if (!localClass) {
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }
        loge("DisplayLink sink: failed to resolve DisplayLinkBridge class");
        displaylinkSinkWriteJavaDiag("rendererInitDisplayLinkBridge: class lookup failed");
        return;
    }

    displayLinkBridgeClass = (*env)->NewGlobalRef(env, localClass);
    (*env)->DeleteLocalRef(env, localClass);
    if (!displayLinkBridgeClass) {
        loge("DisplayLink sink: failed to create global DisplayLinkBridge ref");
        displaylinkSinkWriteJavaDiag("rendererInitDisplayLinkBridge: global ref failed");
        return;
    }

    displayLinkPushFrameMethod = (*env)->GetStaticMethodID(env, displayLinkBridgeClass, "pushFrame", "(Ljava/nio/ByteBuffer;III)Z");
    if (!displayLinkPushFrameMethod) {
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }
        loge("DisplayLink sink: failed to resolve DisplayLinkBridge.pushFrame");
        displaylinkSinkWriteJavaDiag("rendererInitDisplayLinkBridge: pushFrame method lookup failed");
        return;
    }

    displayLinkCaptureReadyMethod = (*env)->GetStaticMethodID(env, displayLinkBridgeClass, "isCaptureReady", "()Z");
    if (!displayLinkCaptureReadyMethod) {
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }
        loge("DisplayLink sink: failed to resolve DisplayLinkBridge.isCaptureReady");
        displaylinkSinkWriteJavaDiag("rendererInitDisplayLinkBridge: isCaptureReady method lookup failed");
        return;
    }

    displayLinkCaptureWidthMethod = (*env)->GetStaticMethodID(env, displayLinkBridgeClass, "getCaptureWidth", "()I");
    if (!displayLinkCaptureWidthMethod) {
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }
        loge("DisplayLink sink: failed to resolve DisplayLinkBridge.getCaptureWidth");
        displaylinkSinkWriteJavaDiag("rendererInitDisplayLinkBridge: getCaptureWidth method lookup failed");
        return;
    }

    displayLinkCaptureHeightMethod = (*env)->GetStaticMethodID(env, displayLinkBridgeClass, "getCaptureHeight", "()I");
    if (!displayLinkCaptureHeightMethod) {
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }
        loge("DisplayLink sink: failed to resolve DisplayLinkBridge.getCaptureHeight");
        displaylinkSinkWriteJavaDiag("rendererInitDisplayLinkBridge: getCaptureHeight method lookup failed");
        return;
    }

    displaylinkSinkWriteJavaDiag("rendererInitDisplayLinkBridge: pushFrame ready");
}

static bool displaylinkSinkCaptureReady(void) {
    JNIEnv* env = NULL;
    bool attached = false;
    jboolean ready = JNI_FALSE;

    if (!rendererVm || !displayLinkBridgeClass || !displayLinkCaptureReadyMethod)
        return false;

    if ((*rendererVm)->GetEnv(rendererVm, (void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        if ((*rendererVm)->AttachCurrentThread(rendererVm, &env, NULL) != JNI_OK)
            return false;
        attached = true;
    }

    ready = (*env)->CallStaticBooleanMethod(env, displayLinkBridgeClass, displayLinkCaptureReadyMethod);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        ready = JNI_FALSE;
    }

    if (attached)
        (*rendererVm)->DetachCurrentThread(rendererVm);
    return ready == JNI_TRUE;
}

static bool displaylinkSinkGetTargetSize(int* width, int* height) {
    JNIEnv* env = NULL;
    bool attached = false;
    jint captureWidth = 0;
    jint captureHeight = 0;

    if (!rendererVm || !displayLinkBridgeClass || !displayLinkCaptureWidthMethod || !displayLinkCaptureHeightMethod)
        return false;

    if ((*rendererVm)->GetEnv(rendererVm, (void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        if ((*rendererVm)->AttachCurrentThread(rendererVm, &env, NULL) != JNI_OK)
            return false;
        attached = true;
    }

    captureWidth = (*env)->CallStaticIntMethod(env, displayLinkBridgeClass, displayLinkCaptureWidthMethod);
    captureHeight = (*env)->CallStaticIntMethod(env, displayLinkBridgeClass, displayLinkCaptureHeightMethod);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        captureWidth = 0;
        captureHeight = 0;
    }

    if (attached)
        (*rendererVm)->DetachCurrentThread(rendererVm);

    if (captureWidth <= 0 || captureHeight <= 0)
        return false;

    *width = captureWidth;
    *height = captureHeight;
    return true;
}

static bool displaylinkSinkEnsureFramebuffer(int width, int height) {
    GLint previousTexture = 0;
    GLint previousFramebuffer = 0;

    if (width <= 0 || height <= 0)
        return false;

    if (!displaylinkSinkFramebuffer)
        glGenFramebuffers(1, &displaylinkSinkFramebuffer);
    if (!displaylinkSinkFramebufferTexture)
        glGenTextures(1, &displaylinkSinkFramebufferTexture);

    if (!displaylinkSinkFramebuffer || !displaylinkSinkFramebufferTexture)
        return false;

    if (displaylinkSinkFramebufferWidth == width && displaylinkSinkFramebufferHeight == height)
        return true;

    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);

    glBindTexture(GL_TEXTURE_2D, displaylinkSinkFramebufferTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glBindFramebuffer(GL_FRAMEBUFFER, displaylinkSinkFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, displaylinkSinkFramebufferTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
        glBindTexture(GL_TEXTURE_2D, previousTexture);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
    glBindTexture(GL_TEXTURE_2D, previousTexture);

    displaylinkSinkFramebufferWidth = width;
    displaylinkSinkFramebufferHeight = height;
    return true;
}

static void displaylinkSinkDrawLetterboxed(GLuint textureId, uint8_t flip, float xfactor,
                                           int sourceWidth, int sourceHeight,
                                           int targetWidth, int targetHeight) {
    float sourceAspect;
    float targetAspect;
    float drawWidth;
    float drawHeight;
    float left;
    float right;
    float top;
    float bottom;

    if (sourceWidth <= 0 || sourceHeight <= 0 || targetWidth <= 0 || targetHeight <= 0)
        return;

    sourceAspect = (float) sourceWidth / (float) sourceHeight;
    targetAspect = (float) targetWidth / (float) targetHeight;

    if (sourceAspect > targetAspect) {
        drawWidth = 2.f;
        drawHeight = 2.f * (targetAspect / sourceAspect);
    } else {
        drawHeight = 2.f;
        drawWidth = 2.f * (sourceAspect / targetAspect);
    }

    left = -drawWidth * 0.5f;
    right = drawWidth * 0.5f;
    top = -drawHeight * 0.5f;
    bottom = drawHeight * 0.5f;
    draw(textureId, left, top, right, bottom, xfactor, flip);
}

static void displaylinkSinkDrawCursorLetterboxed(int sourceWidth, int sourceHeight, int targetWidth, int targetHeight) {
    float sourceAspect;
    float targetAspect;
    float drawWidth;
    float drawHeight;
    float left;
    float top;
    float right;
    float bottom;
    float cursorLeft;
    float cursorTop;
    float cursorWidth;
    float cursorHeight;

    if (!state || !state->cursor.width || !state->cursor.height ||
        sourceWidth <= 0 || sourceHeight <= 0 || targetWidth <= 0 || targetHeight <= 0) {
        return;
    }

    sourceAspect = (float) sourceWidth / (float) sourceHeight;
    targetAspect = (float) targetWidth / (float) targetHeight;

    if (sourceAspect > targetAspect) {
        drawWidth = 2.f;
        drawHeight = 2.f * (targetAspect / sourceAspect);
    } else {
        drawHeight = 2.f;
        drawWidth = 2.f * (sourceAspect / targetAspect);
    }

    left = -drawWidth * 0.5f;
    top = -drawHeight * 0.5f;
    right = left + drawWidth;
    bottom = top + drawHeight;

    cursorLeft = left + drawWidth * (((float) state->cursor.x - (float) state->cursor.xhot) / (float) sourceWidth);
    cursorTop = top + drawHeight * (((float) state->cursor.y - (float) state->cursor.yhot) / (float) sourceHeight);
    cursorWidth = drawWidth * ((float) state->cursor.width / (float) sourceWidth);
    cursorHeight = drawHeight * ((float) state->cursor.height / (float) sourceHeight);

    if (cursorWidth > drawWidth)
        cursorWidth = drawWidth;
    if (cursorHeight > drawHeight)
        cursorHeight = drawHeight;

    if (cursorLeft < left)
        cursorLeft = left;
    else if (cursorLeft + cursorWidth > right)
        cursorLeft = right - cursorWidth;

    if (cursorTop < top)
        cursorTop = top;
    else if (cursorTop + cursorHeight > bottom)
        cursorTop = bottom - cursorHeight;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw(cursor.id, cursorLeft, cursorTop, cursorLeft + cursorWidth, cursorTop + cursorHeight, 1.f, false);
    glDisable(GL_BLEND);
}

static bool displaylinkSinkRenderAndPostFramebuffer(LorieBuffer* buffer, const LorieBuffer_Desc* desc, float xfactor, int width, int height) {
    GLint previousFramebuffer = 0;
    GLint previousViewport[4] = {0};
    bool posted;

    if (!displaylinkSinkEnsureFramebuffer(width, height))
        return false;

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, displaylinkSinkFramebuffer);
    glViewport(0, 0, width, height);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    LorieBuffer_bindTexture(buffer);
    displaylinkSinkDrawLetterboxed(0, LorieBuffer_isRgba(buffer), xfactor,
            desc->width, desc->height, width, height);
    displaylinkSinkDrawCursorLetterboxed(desc->width, desc->height, width, height);
    glFlush();

    posted = displaylinkSinkPostCurrentFramebuffer(width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    return posted;
}

static void displaylinkSinkDumpFrame(const uint8_t* pixels, int width, int height) {
    int fd;
    char path[256];
    char header[128];
    int headerLen;
    const char* outDir;

    if (!pixels || width <= 0 || height <= 0)
        return;

    outDir = displaylinkSinkResolveOutputDir();
    snprintf(path, sizeof(path), "%s/frame-%06d.ppm", outDir, displaylinkSinkFrameCounter);
    fd = open(path, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, 0644);
    if (fd < 0) {
        log("DisplayLink sink: failed to open %s: %s", path, strerror(errno));
        return;
    }

    headerLen = snprintf(header, sizeof(header), "P6\n%d %d\n255\n", width, height);
    if (write(fd, header, headerLen) != headerLen) {
        close(fd);
        return;
    }

    for (int y = height - 1; y >= 0; --y) {
        const uint8_t* row = pixels + ((size_t) y * (size_t) width * 4);
        for (int x = 0; x < width; ++x) {
            const uint8_t* src = row + ((size_t) x * 4);
            uint8_t pixel[3] = { src[0], src[1], src[2] };
            if (write(fd, pixel, sizeof(pixel)) != sizeof(pixel)) {
                close(fd);
                return;
            }
        }
    }

    log("DisplayLink sink: dumped framebuffer %d (%dx%d) to %s",
        displaylinkSinkFrameCounter, width, height, path);
    displaylinkSinkFrameCounter++;

    close(fd);
}

static void displaylinkSinkDumpCurrentFramebuffer(int width, int height) {
    uint8_t* pixels;

    if (width <= 0 || height <= 0)
        return;

    pixels = calloc((size_t) width * (size_t) height, 4);
    if (!pixels) {
        loge("DisplayLink sink: failed to allocate framebuffer dump buffer for %dx%d", width, height);
        return;
    }

    log("DisplayLink sink: reading framebuffer %dx%d", width, height);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if (glGetError() != GL_NO_ERROR) {
        loge("DisplayLink sink: glReadPixels failed for %dx%d", width, height);
        free(pixels);
        return;
    }

    log("DisplayLink sink: framebuffer read completed for %dx%d", width, height);
    displaylinkSinkDumpFrame(pixels, width, height);
    free(pixels);
}

static bool displaylinkSinkPostFrameToJava(const uint8_t* pixels, int width, int height, int rowStride) {
    JNIEnv* env = NULL;
    bool attached = false;
    jobject buffer = NULL;
    jboolean pushed = JNI_FALSE;
    size_t bufferSize;

    if (!rendererVm || !displayLinkBridgeClass || !displayLinkPushFrameMethod || !pixels || width <= 0 || height <= 0 || rowStride <= 0) {
        if (displaylinkSinkJavaDiagCount < 20) {
            char diag[256];
            snprintf(diag, sizeof(diag),
                     "postFrameToJava prereq failed vm=%d class=%d method=%d pixels=%d width=%d height=%d rowStride=%d",
                     rendererVm != NULL,
                     displayLinkBridgeClass != NULL,
                     displayLinkPushFrameMethod != NULL,
                     pixels != NULL,
                     width,
                     height,
                     rowStride);
            displaylinkSinkWriteJavaDiag(diag);
            displaylinkSinkJavaDiagCount++;
        }
        return false;
    }

    if ((*rendererVm)->GetEnv(rendererVm, (void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        if ((*rendererVm)->AttachCurrentThread(rendererVm, &env, NULL) != JNI_OK)
            return false;
        attached = true;
    }

    bufferSize = (size_t) rowStride * (size_t) height;
    buffer = (*env)->NewDirectByteBuffer(env, (void*) pixels, (jlong) bufferSize);
    if (!buffer)
        goto done;

    if (displaylinkSinkJavaDiagCount < 20) {
        char diag[256];
        snprintf(diag, sizeof(diag), "postFrameToJava invoking width=%d height=%d rowStride=%d size=%zu", width, height, rowStride, bufferSize);
        displaylinkSinkWriteJavaDiag(diag);
        displaylinkSinkJavaDiagCount++;
    }

    pushed = (*env)->CallStaticBooleanMethod(env, displayLinkBridgeClass, displayLinkPushFrameMethod, buffer, width, height, rowStride);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        pushed = JNI_FALSE;
        displaylinkSinkWriteJavaDiag("postFrameToJava exception from CallStaticBooleanMethod");
    } else if (displaylinkSinkJavaDiagCount < 20) {
        displaylinkSinkWriteJavaDiag(pushed == JNI_TRUE ? "postFrameToJava returned true" : "postFrameToJava returned false");
        displaylinkSinkJavaDiagCount++;
    }

done:
    if (buffer)
        (*env)->DeleteLocalRef(env, buffer);
    if (attached)
        (*rendererVm)->DetachCurrentThread(rendererVm);
    return pushed == JNI_TRUE;
}

static bool displaylinkSinkPostCurrentFramebuffer(int width, int height) {
    uint8_t* pixels;
    bool posted;
    int rowStride;
    size_t requiredSize;

    if (width <= 0 || height <= 0)
        return false;

    rowStride = width * 4;
    requiredSize = (size_t) rowStride * (size_t) height;
    if (displaylinkSinkPostBuffer == NULL || displaylinkSinkPostBufferSize < requiredSize) {
        uint8_t* replacement = realloc(displaylinkSinkPostBuffer, requiredSize);
        if (!replacement) {
            loge("DisplayLink sink: failed to grow framebuffer post buffer for %dx%d", width, height);
            return false;
        }
        displaylinkSinkPostBuffer = replacement;
        displaylinkSinkPostBufferSize = requiredSize;
    }

    pixels = displaylinkSinkPostBuffer;
    if (!pixels) {
        loge("DisplayLink sink: failed to allocate framebuffer post buffer for %dx%d", width, height);
        return false;
    }

    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if (glGetError() != GL_NO_ERROR) {
        loge("DisplayLink sink: glReadPixels failed for postFrame %dx%d", width, height);
        return false;
    }

    posted = displaylinkSinkPostFrameToJava(pixels, width, height, rowStride);
    if (!posted)
        log("DisplayLink sink: Java postFrame path not ready for %dx%d", width, height);
    return posted;
}

bool displaylinkSinkFrame(const struct lorie_shared_server_state* sharedState, LorieBuffer* buffer, const LorieBuffer_Desc* desc,
                          bool drawRequested, bool cursorUpdated, bool cursorMoved) {
    static uint64_t lastFrameId = 0;
    static uint8_t lastCursorUpdated = 0;
    static uint8_t lastCursorMoved = 0;
    int targetWidth = 0;
    int targetHeight = 0;

    if (!displaylinkSinkEnabled() || !sharedState || !buffer || !desc) {
        log("DisplayLink sink skipped: enabled=%d state=%p buffer=%p desc=%p",
            displaylinkSinkEnabled(), sharedState, buffer, desc);
        return false;
    }

    if (!displaylinkSinkCaptureReady()) {
        return false;
    }

    if (!displaylinkSinkGetTargetSize(&targetWidth, &targetHeight)) {
        return false;
    }

    if (displaylinkSinkMaxFrames > 0 && displaylinkSinkFrameCounter >= displaylinkSinkMaxFrames) {
        log("DisplayLink sink skipped: frame limit reached (%d/%d)",
            displaylinkSinkFrameCounter, displaylinkSinkMaxFrames);
        return false;
    }

    if (!drawRequested &&
        lastFrameId == desc->id &&
        lastCursorUpdated == cursorUpdated &&
        lastCursorMoved == cursorMoved) {
        log("DisplayLink sink skipped: no new frame state for id=%llu",
            (unsigned long long) desc->id);
        return false;
    }

    lastFrameId = desc->id;
    lastCursorUpdated = cursorUpdated;
    lastCursorMoved = cursorMoved;

    if (displaylinkSinkCaptureInProgress) {
        displaylinkSinkCapturePending = true;
        displaylinkSinkCaptureWidth = targetWidth;
        displaylinkSinkCaptureHeight = targetHeight;
        return false;
    }

    log("DisplayLink sink frame candidate: id=%llu size=%dx%d stride=%d fmt=%d type=%d draw=%d cursorU=%d cursorM=%d out=%s",
        (unsigned long long) desc->id,
        desc->width,
        desc->height,
        desc->stride,
        desc->format,
        desc->type,
        drawRequested,
        cursorUpdated,
        cursorMoved,
        displaylinkSinkResolveOutputDir());
    displaylinkSinkAttemptCounter++;
    log("DisplayLink sink attempt #%d queued", displaylinkSinkAttemptCounter);

    displaylinkSinkCapturePending = true;
    displaylinkSinkCaptureWidth = targetWidth;
    displaylinkSinkCaptureHeight = targetHeight;
    return true;
}

static inline __always_inline void bindTexture(GLuint id) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_NONE
};

const EGLint ctxattribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
};

static void onImageAvailable(void* context, AImageReader* reader) {
    AImage* image = NULL;
    if (AImageReader_acquireLatestImage(reader, &image) == AMEDIA_OK && image)
        AImage_delete(image);
}

int rendererInitThread(void) {
    EGLint major, minor;
    EGLint numConfigs;
    EGLint *const alphaAttrib = &configAttribs[11];
    AImageReader* reader = NULL; // We will use this ImageReader each time surface is destroyed, zero reasons to clean it up

    pthread_setname_np(pthread_self(), "LorieRendererThread");

    displaylinkSinkFrameCounter = 0;
    displaylinkSinkAttemptCounter = 0;
    displaylinkSinkCapturePending = false;
    displaylinkSinkCaptureInProgress = false;
    displaylinkSinkCaptureWidth = 0;
    displaylinkSinkCaptureHeight = 0;
    if (displaylinkSinkFramebuffer)
        glDeleteFramebuffers(1, &displaylinkSinkFramebuffer);
    if (displaylinkSinkFramebufferTexture)
        glDeleteTextures(1, &displaylinkSinkFramebufferTexture);
    displaylinkSinkFramebuffer = 0;
    displaylinkSinkFramebufferTexture = 0;
    displaylinkSinkFramebufferWidth = 0;
    displaylinkSinkFramebufferHeight = 0;
    free(displaylinkSinkPostBuffer);
    displaylinkSinkPostBuffer = NULL;
    displaylinkSinkPostBufferSize = 0;

    xorg_list_init(&addedBuffers);
    xorg_list_init(&buffers);
    xorg_list_init(&removedBuffers);

    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY)
        return printEglError("Got no EGL display", __LINE__);

    if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE)
        return printEglError("Unable to initialize EGL", __LINE__);

    log("Xlorie: Initialized EGL version %d.%d\n", major, minor);
    eglBindAPI(EGL_OPENGL_ES_API);

    if (eglChooseConfig(egl_display, configAttribs, &cfg, 1, &numConfigs) != EGL_TRUE &&
        (*alphaAttrib = 8) &&
        eglChooseConfig(egl_display, configAttribs, &cfg, 1, &numConfigs) != EGL_TRUE)
        return printEglError("eglChooseConfig failed", __LINE__);

    ctx = eglCreateContext(egl_display, cfg, NULL, ctxattribs);
    if (ctx == EGL_NO_CONTEXT)
        return printEglError("eglCreateContext failed", __LINE__);

    // Weird devices without proper EGL_KHR_surfaceless_context support
    // We can not use pbuffer-based surfaces because it will require searching for configs supporting it
    // and I am not sure all devices have configs supporting both pbuffers and regular surfaces simultaneously
    if (AImageReader_new(1, 1, AIMAGE_FORMAT_RGBA_8888, 2, &reader) != AMEDIA_OK) {
        log("Failed to initialise ImageReader");
        return 1;
    }

    if (AImageReader_setImageListener(reader, &(AImageReader_ImageListener) { .context = NULL, .onImageAvailable = onImageAvailable }) != AMEDIA_OK) {
        log("Failed to set ImageReader listener");
        AImageReader_delete(reader);
        return 1;
    }

    if (AImageReader_getWindow(reader, &defaultWin) != AMEDIA_OK) {
        log("Failed to obtain ImageReader native window");
        AImageReader_delete(reader);
        return 1;
    }

    win = defaultWin;
    ANativeWindow_acquire(defaultWin);

    sfc = defaultSfc = eglCreateWindowSurface(egl_display, cfg, win, NULL);

    eglMakeCurrent(egl_display, sfc, sfc, ctx);
    eglSwapInterval(egl_display, 0);

    g_texture_program = createProgram(vertexShaderSrc, fragmentShaderSrc);
    if (!g_texture_program)
        log("Xlorie: GLESv2: Unable to create shader program.\n");

    g_texture_program_bgra = createProgram(vertexShaderSrc, fragmentShaderBgraSrc);
    if (!g_texture_program_bgra)
        log("Xlorie: GLESv2: Unable to create bgra shader program.\n");

    gv_pos = (GLuint) glGetAttribLocation(g_texture_program, "position");
    gv_coords = (GLuint) glGetAttribLocation(g_texture_program, "texCoords");

    gv_pos_bgra = (GLuint) glGetAttribLocation(g_texture_program_bgra, "position");
    gv_coords_bgra = (GLuint) glGetAttribLocation(g_texture_program_bgra, "texCoords");

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &cursor.id);

    rendererThread();
    return 1;
}

void rendererInit(JNIEnv* env) {
    pthread_t t;

    if (ctx)
        return;

    pthreadCondVarProxyInit();

    pthread_mutex_init(&stateLock, NULL);
    pthread_cond_init(&stateCond, NULL);
    pthread_cond_init(&stateChangeFinishCond, NULL);
    pthread_spin_init(&bufferLock, false);

    pthread_create(&t, NULL, (void*(*)(void*)) rendererInitThread, NULL);
}
void rendererSetFiltering(JNIEnv* env, jobject self, jint f) {
    filtering = f;
}

void rendererTestCapabilities(int* legacy_drawing, uint8_t* flip) {
    // Some devices do not support sampling from HAL_PIXEL_FORMAT_BGRA_8888, here we are checking it.
    const EGLint imageAttributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
    EGLint numConfigs;
    EGLClientBuffer clientBuffer;
    EGLImageKHR img;
    AHardwareBuffer *new = NULL;
    int status;
    AHardwareBuffer_Desc d0 = {
            .width = 64,
            .height = 64,
            .layers = 1,
            .usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            .format = AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM
    };

    if (egl_display == EGL_NO_DISPLAY) {
        egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (egl_display == EGL_NO_DISPLAY)
            return vprintEglError("Got no EGL display", __LINE__);
    }

    status = AHardwareBuffer_allocate(&d0, &new);
    if (status != 0 || new == NULL) {
        loge("Failed to allocate native buffer (%p, error %d)", new, status);
        loge("Forcing legacy drawing");
        *legacy_drawing = 1;
        return;
    }

    uint32_t *pixels;
    if (AHardwareBuffer_lock(new, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, NULL, (void **) &pixels) == 0) {
        pixels[0] = 0xAABBCCDD;
        AHardwareBuffer_unlock(new, NULL);
    } else {
        loge("Failed to lock native buffer (%p, error %d)", new, status);
        loge("Forcing legacy drawing");
        *legacy_drawing = 1;
        AHardwareBuffer_release(new);
        return;
    }

    clientBuffer = eglGetNativeClientBufferANDROID(new);
    if (!clientBuffer) {
        *legacy_drawing = 1;
        AHardwareBuffer_release(new);
        return vprintEglError("Failed to obtain EGLClientBuffer from AHardwareBuffer, forcing legacy drawing", __LINE__);
    }

    if (!(img = eglCreateImageKHR(egl_display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, imageAttributes))) {
        if (eglGetError() == EGL_BAD_PARAMETER) {
            loge("Sampling from HAL_PIXEL_FORMAT_BGRA_8888 is not supported, forcing AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM");
            *flip = 1;
        } else {
            loge("Failed to obtain EGLImageKHR from EGLClientBuffer");
            loge("Forcing legacy drawing");
            *legacy_drawing = 1;
        }
        AHardwareBuffer_release(new);
    } else {
        // For some reason all devices I checked had no GL_EXT_texture_format_BGRA8888 support, but some of them still provided BGRA extension.
        // EGL does not provide functions to query texture format in runtime.
        // Workarounds are less performant but at least they let us use Termux:X11 on devices with missing BGRA support.
        // We handle two cases.
        // If resulting texture has BGRA format but still drawing RGBA we should flip format to RGBA and flip pixels manually in shader.
        // In the case if for some reason we can not use HAL_PIXEL_FORMAT_BGRA_8888 we should fallback to legacy drawing method (uploading pixels via glTexImage2D).
        configAttribs[1] = EGL_PBUFFER_BIT;
        EGLConfig checkcfg = 0;
        GLuint fbo = 0, texture = 0;
        if (eglChooseConfig(egl_display, configAttribs, &checkcfg, 1, &numConfigs) != EGL_TRUE)
            return vprintEglError("check eglChooseConfig failed", __LINE__);

        EGLContext testctx = eglCreateContext(egl_display, checkcfg, NULL, ctxattribs);
        if (testctx == EGL_NO_CONTEXT)
            return vprintEglError("check eglCreateContext failed", __LINE__);

        const EGLint pbufferAttributes[] = {
                EGL_WIDTH, 64,
                EGL_HEIGHT, 64,
                EGL_NONE,
        };
        EGLSurface checksfc = eglCreatePbufferSurface(egl_display, checkcfg, pbufferAttributes);

        if (eglMakeCurrent(egl_display, checksfc, checksfc, testctx) != EGL_TRUE)
            return vprintEglError("check eglMakeCurrent failed", __LINE__);

        glActiveTexture(GL_TEXTURE0); checkGlError();
        glGenTextures(1, &texture); checkGlError();
        bindTexture(texture);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, img); checkGlError();
        glGenFramebuffers(1, &fbo); checkGlError();
        glBindFramebuffer(GL_FRAMEBUFFER, fbo); checkGlError();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0); checkGlError();
        uint32_t pixel[64*64];
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel); checkGlError();
        if (pixel[0] == 0xAABBCCDD) {
            log("Xlorie: GLES draws pixels unchanged, probably system does not support AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM. Forcing bgra.\n");
            *flip = 1;
        } else if (pixel[0] != 0xAADDCCBB) {
            log("Xlorie: GLES receives broken pixels. Forcing legacy drawing. 0x%X\n", pixel[0]);
            *legacy_drawing = 1;
        }
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(egl_display, testctx);
        eglDestroyImageKHR(egl_display, img);
        eglDestroySurface(egl_display, checksfc);
        AHardwareBuffer_release(new);
    }
}

__unused void rendererSetSharedState(struct lorie_shared_server_state* newState) {
    pthread_mutex_lock(&stateLock);
    pendingState = newState;
    stateChanged = true;
    pthread_cond_signal(&stateCond);

    while(stateChanged)
        pthread_cond_wait(&stateChangeFinishCond, &stateLock);

    pthread_mutex_unlock(&stateLock);
}

void rendererAddBuffer(LorieBuffer* buf) {
    pthread_spin_lock(&bufferLock);
    LorieBuffer_addToList(buf, &addedBuffers);
    pthread_cond_signal(&stateCond);
    pthread_spin_unlock(&bufferLock);
}

void rendererRemoveBuffer(uint64_t id) {
    pthread_spin_lock(&bufferLock);
    LorieBuffer* buf = LorieBufferList_findById(&addedBuffers, id);
    if (buf)
        // Buffer was not attached to GL yet, it is safe to release it now.
        LorieBuffer_release(buf);
    else {
        buf = LorieBufferList_findById(&buffers, id);
        if (buf) {
            // The buffer is attached to GL so we should release it from renderer thread.
            LorieBuffer_removeFromList(buf);
            LorieBuffer_addToList(buf, &removedBuffers);
        }
    }
    pthread_spin_unlock(&bufferLock);
}

void rendererRemoveAllBuffers(void) {
    LorieBuffer *buf = NULL;

    pthread_spin_lock(&bufferLock);
    while ((buf = LorieBufferList_first(&addedBuffers))) {
        // These buffers are not yet attached to GL, it is safe to release them
        LorieBuffer_release(buf);
    }
    while ((buf = LorieBufferList_first(&buffers))) {
        // These buffers are attached to GL, we must release them from renderer thread.
        LorieBuffer_removeFromList(buf);
        LorieBuffer_addToList(buf, &removedBuffers);
    }
    pthread_spin_unlock(&bufferLock);
}

void rendererSetWindow(ANativeWindow* newWin) {
    pthread_mutex_lock(&stateLock);
    if (newWin && pendingWin == newWin) {
        ANativeWindow_release(newWin);
        pthread_mutex_unlock(&stateLock);
        return;
    }

    if (pendingWin)
        ANativeWindow_release(pendingWin);

    pendingWin = newWin;
    windowChanged = TRUE;

    pthread_cond_signal(&stateCond);

    // We should wait until renderer destroys EGLSurface before SurfaceCallback::surfaceDestroyed finishes
    // Otherwise we will have weird errors like
    // `freeAllBuffers: 1 buffers were freed while being dequeued!`
    // or
    // `query: BufferQueue has been abandoned`
    while(windowChanged)
        pthread_cond_wait(&stateChangeFinishCond, &stateLock);

    pthread_mutex_unlock(&stateLock);
}

static inline __always_inline void releaseWinAndSurface(ANativeWindow** anw, EGLSurface *esfc) {
    if (esfc && *esfc && *esfc != defaultSfc) {
        // Requeue the dequeued buffer, causes flickering during window reconfiguring
        eglSwapBuffers(egl_display, *esfc);
        if (eglMakeCurrent(egl_display, defaultSfc, defaultSfc, ctx) != EGL_TRUE)
            return vprintEglError("eglMakeCurrent failed (EGL_NO_SURFACE)", __LINE__);
        if (eglDestroySurface(egl_display, *esfc) != EGL_TRUE)
            return vprintEglError("eglDestoySurface failed", __LINE__);
        *esfc = defaultSfc;
    }

    if (anw && *anw && *anw != defaultWin) {
        ANativeWindow_release(*anw);
        *anw = defaultWin;
    }
}

void rendererRefreshContext(void) {
    int width = pendingWin ? ANativeWindow_getWidth(pendingWin) : 0;
    int height = pendingWin ? ANativeWindow_getHeight(pendingWin) : 0;
    log("rendererSetWindow %p %d %d", pendingWin, width, height);

    releaseWinAndSurface(&win, &sfc);

    if (pendingWin && (width <= 0 || height <= 0)) {
        log("Xlorie: We've got invalid surface. Probably it became invalid before we started working with it.\n");
        releaseWinAndSurface(&pendingWin, NULL);
    }

    win = pendingWin;
    pendingWin = NULL;
    windowChanged = FALSE;

    if (!win) {
        win = defaultWin;
        eglMakeCurrent(egl_display, defaultSfc, defaultSfc, ctx);
        if (state)
            state->surfaceAvailable = false;
        return;
    }

    sfc = eglCreateWindowSurface(egl_display, cfg, win, NULL);
    if (sfc == EGL_NO_SURFACE)
        return vprintEglError("eglCreateWindowSurface failed", __LINE__);

    if (eglMakeCurrent(egl_display, sfc, sfc, ctx) != EGL_TRUE) {
        if (state)
            state->surfaceAvailable = false;
        return vprintEglError("eglMakeCurrent failed", __LINE__);
    }

    eglSwapInterval(egl_display, 0);

    // We should redraw image at least once right after surface change
    if (state)
        state->surfaceAvailable = state->drawRequested = state->cursor.updated = win != defaultWin;

    glViewport(0, 0, ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));
    log("Xlorie: new surface applied: %p\n", sfc);
}

static void draw(GLuint id, float x0, float y0, float x1, float y1, float xfactor, uint8_t flip);
static void drawCursor(float displayWidth, float displayHeight);

void rendererRedrawLocked(bool* waitingForBuffers) {
    float xfactor = 1.f;
    const LorieBuffer_Desc *desc = NULL;
    LorieBuffer_Desc descStorage;
    EGLSync fence;
    bool drawRequested = false;
    bool cursorUpdated = false;
    bool cursorMoved = false;
    // The buffer will not be released until this function ends, but main thread can modify buffer list
    pthread_spin_lock(&bufferLock);
    LorieBuffer *buffer = LorieBufferList_findById(&buffers, state->rootWindowTextureID);
    // Probably X server requested us to draw removed buffer and immediately requested to remove it. Let's display it one last time.
    if (!buffer)
        buffer = LorieBufferList_findById(&removedBuffers, state->rootWindowTextureID);
    if (!buffer)
        *waitingForBuffers = true;
    else
        LorieBuffer_acquire(buffer);
    pthread_spin_unlock(&bufferLock);
    if (!buffer) {
        log("Buffer %llu not found", state->rootWindowTextureID);
        return;
    }

    descStorage = *LorieBuffer_description(buffer);
    desc = &descStorage;

    // We should signal X server to not use root window while we actively copy it
    lorie_mutex_lock(&state->lock, &state->lockingPid);
    drawRequested = state->drawRequested;
    cursorUpdated = state->cursor.updated;
    cursorMoved = state->cursor.moved;
    log("rendererRedrawLocked: id=%llu surface=%d draw=%d cursorU=%d cursorM=%d",
        (unsigned long long) state->rootWindowTextureID,
        state->surfaceAvailable,
        drawRequested,
        cursorUpdated,
        cursorMoved);
    state->drawRequested = FALSE;

    displaylinkSinkFrame((const struct lorie_shared_server_state*) state, buffer, desc, drawRequested, cursorUpdated, cursorMoved);

    if (state->surfaceAvailable) {
        LorieBuffer_bindTexture(buffer);
        if (desc->type == LORIEBUFFER_FD)
            xfactor = (float) desc->width/(float) desc->stride;
        draw(0, -1.f, -1.f, 1.f, 1.f, xfactor, LorieBuffer_isRgba(buffer));
        fence = eglCreateSyncKHR(egl_display, EGL_SYNC_FENCE_KHR, NULL);
        glFlush();

        if (state->cursor.updated) {
            log("Xlorie: updating cursor\n");
            lorie_mutex_lock(&state->cursor.lock, &state->cursor.lockingPid);
            state->cursor.updated = false;
            bindTexture(cursor.id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei) state->cursor.width, (GLsizei) state->cursor.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, state->cursor.bits);
            lorie_mutex_unlock(&state->cursor.lock, &state->cursor.lockingPid);
        }

        state->cursor.moved = FALSE;
        drawCursor((float) (LorieBuffer_getWidth(buffer)), (float) (LorieBuffer_getHeight(buffer)));
        glFlush();
        if (displaylinkSinkCapturePending) {
            displaylinkSinkCaptureInProgress = true;
            if (!displaylinkSinkRenderAndPostFramebuffer(buffer, desc, xfactor,
                    displaylinkSinkCaptureWidth, displaylinkSinkCaptureHeight))
                displaylinkSinkDumpCurrentFramebuffer(displaylinkSinkCaptureWidth, displaylinkSinkCaptureHeight);
            displaylinkSinkCaptureInProgress = false;
            displaylinkSinkCapturePending = false;
        }

        // Wait until root window drawing is finished before giving control back to X server
        eglClientWaitSyncKHR(egl_display, fence, 0, EGL_FOREVER);
        eglDestroySyncKHR(egl_display, fence);
        state->waitForNextFrame = true;
    } else {
        displaylinkSinkCapturePending = false;
        displaylinkSinkCaptureInProgress = false;
        state->cursor.updated = FALSE;
        state->cursor.moved = FALSE;
        state->waitForNextFrame = false;
    }
    lorie_mutex_unlock(&state->lock, &state->lockingPid);

    if (state->surfaceAvailable) {
        if (eglSwapBuffers(egl_display, sfc) != EGL_TRUE)
            printEglError("Failed to swap buffers", __LINE__);

        // Perform a little drawing operation to make sure the next buffer is ready on the next invocation of drawing
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, 1, 1);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
        fence = eglCreateSyncKHR(egl_display, EGL_SYNC_FENCE_KHR, NULL);
        eglClientWaitSyncKHR(egl_display, fence, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, EGL_FOREVER);
        eglDestroySyncKHR(egl_display, fence);
    }

    state->renderedFrames++;
    LorieBuffer_release(buffer);
}

static inline __always_inline bool rendererShouldWait(bool *waitingForBuffers) {
    static uint64_t lastRequestedBufferId = 0;
    bool buffersChanged;
    pthread_spin_lock(&bufferLock);
    buffersChanged = !xorg_list_is_empty(&addedBuffers) || !xorg_list_is_empty(&removedBuffers);
    pthread_spin_unlock(&bufferLock);
    if (stateChanged || windowChanged || buffersChanged)
        // If there are pending changes we should process them immediately.
        return false;

    if (state) {
        if (lastRequestedBufferId != state->rootWindowTextureID)
            *waitingForBuffers = false;
        lastRequestedBufferId = state->rootWindowTextureID;
    }

    if (!state || ((!state->surfaceAvailable && !displaylinkSinkEnabled())) || state->waitForNextFrame || *waitingForBuffers)
        // Even in the case if there are pending changes, we can not draw it without rendering surface
        return true;

    if (state->drawRequested || state->cursor.moved || state->cursor.updated)
        // X server reported drawing or cursor changes, no need to wait.
        return false;

    // Probably spurious wake, no changes we can work with.
    return true;
}

__noreturn static void* rendererThread(void) {
    LorieBuffer* buf;
    bool waitingForBuffers = false;
    while (true) {
        while (rendererShouldWait(&waitingForBuffers))
            pthread_cond_wait(&stateCond, &stateLock);

        if (stateChanged) {
            struct lorie_shared_server_state* oldState = NULL;
            if (state && pendingState != state)
                oldState = state;

            state = pendingState;
            pendingState = NULL;
            stateChanged = false;
            waitingForBuffers = false;

            if (state)
                state->surfaceAvailable = win != defaultWin;
            else if (win != defaultWin) {
                glClearColor(0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT);
                eglSwapBuffers(egl_display, sfc);
            }

            pthreadCondVarProxyListenOtherCondVar(state ? &state->cond : NULL);

            if (oldState)
                munmap(oldState, sizeof(*oldState));
        }

        if (windowChanged)
            rendererRefreshContext();

        // Attach all pending buffers to GL.
        pthread_spin_lock(&bufferLock);
        while((buf = LorieBufferList_first(&addedBuffers))) {
            LorieBuffer_attachToGL(buf);
            LorieBuffer_addToList(buf, &buffers);
            if (displaylinkSinkEnabled() && displaylinkSinkProbeCount < 8) {
                const LorieBuffer_Desc* attachedDesc = LorieBuffer_description(buf);
                log("DisplayLink sink probe on attached buffer: id=%llu size=%dx%d stride=%d fmt=%d type=%d",
                    (unsigned long long) attachedDesc->id,
                    attachedDesc->width,
                    attachedDesc->height,
                    attachedDesc->stride,
                    attachedDesc->format,
                    attachedDesc->type);
                displaylinkSinkProbeCount++;
            }
            waitingForBuffers = false;
        }
        pthread_spin_unlock(&bufferLock);

        pthread_cond_signal(&stateChangeFinishCond);
        pthread_mutex_unlock(&stateLock);

        if (state && (state->surfaceAvailable || displaylinkSinkEnabled()) && !state->waitForNextFrame && (state->drawRequested || state->cursor.moved || state->cursor.updated))
            rendererRedrawLocked(&waitingForBuffers);

        pthread_spin_lock(&bufferLock);
        // Remove all buffers which were attached to GL.
        while((buf = LorieBufferList_first(&removedBuffers)))
            LorieBuffer_release(buf);
        pthread_spin_unlock(&bufferLock);
        pthread_mutex_lock(&stateLock);
    }
}

static GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLint compiled = 0, infoLen = 0;
    GLuint shader = glCreateShader(shaderType);
    if (!shader)
        return 0;

    glShaderSource(shader, 1, &pSource, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled)
        return shader;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen) {
        char buf[infoLen];
        glGetShaderInfoLog(shader, infoLen, NULL, buf);
        log("renderer: Could not compile shader %d:\n%s\n", shaderType, buf);
    }
    glDeleteShader(shader);

    return 0;
}

static GLuint createProgram(const char* p_vertex_source, const char* p_fragment_source) {
    GLuint program, vertexShader, pixelShader;
    GLint linkStatus = GL_FALSE, bufLength = 0;
    vertexShader = loadShader(GL_VERTEX_SHADER, p_vertex_source);
    pixelShader = loadShader(GL_FRAGMENT_SHADER, p_fragment_source);
    if (!pixelShader || !vertexShader) {
        return 0;
    }

    program = glCreateProgram();
    if (!program)
        return 0;

    glAttachShader(program, vertexShader);
    glAttachShader(program, pixelShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_TRUE)
        return program;

    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
    if (bufLength) {
        char buf[bufLength];
        glGetProgramInfoLog(program, bufLength, NULL, buf);
        log("renderer: Could not link program:\n%s\n", buf);
    }
    glDeleteProgram(program);

    return 0;
}

static void draw(GLuint id, float x0, float y0, float x1, float y1, float xfactor, uint8_t flip) {
    float coords[16] = {
        x0, -y0, 0.f, 0.f,
        x1, -y0, xfactor, 0.f,
        x0, -y1, 0.f, 1.f,
        x1, -y1, xfactor, 1.f,
    };

    GLuint p = flip ? gv_pos_bgra : gv_pos, c = flip ? gv_coords_bgra : gv_coords;

    glActiveTexture(GL_TEXTURE0);
    glUseProgram(flip ? g_texture_program_bgra : g_texture_program);
    if (id)
        glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
    glVertexAttribPointer(p, 2, GL_FLOAT, GL_FALSE, 16, coords);
    glVertexAttribPointer(c, 2, GL_FLOAT, GL_FALSE, 16, &coords[2]);
    glEnableVertexAttribArray(p);
    glEnableVertexAttribArray(c);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); checkGlError();
}

__unused static void drawCursor(float displayWidth, float displayHeight) {
    float x, y, w, h;

    if (!state->cursor.width || !state->cursor.height)
        return;

    x = 2.f * ((float) state->cursor.x - (float) state->cursor.xhot) / displayWidth - 1.f;
    y = 2.f * ((float) state->cursor.y - (float) state->cursor.yhot) / displayHeight - 1.f;
    w = 2.f * (float) state->cursor.width / displayWidth;
    h = 2.f * (float) state->cursor.height / displayHeight;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw(cursor.id, x, y, x + w, y + h, 1.f, false);
    glDisable(GL_BLEND);
}

// auxillary pthread condition var proxy shenanigans
static volatile struct {
    pthread_mutex_t lock;
    pthread_cond_t def, *current, *pending;
    bool relocked;
} proxy = { .current = &proxy.def };

static void pthreadCondVarProxyInit(void) {
    pthread_t t;
    pthread_mutex_init(&proxy.lock, NULL);
    pthread_cond_init(&proxy.def, NULL);
    pthread_create(&t, NULL, pthreadCondVarProxyThread, NULL);
}

__noreturn static void* pthreadCondVarProxyThread(void* cookie) {
    // We can not wait for two conditional variables simultaneously.
    // But we are required to listen for both remote and local events.
    // This thread waits for remote signals and proxies them to the local cond var.
    pthread_setname_np(pthread_self(), "PthreadCondVarProxy");
    pthread_mutex_lock(&proxy.lock);
    log("pthreadCondVarProxyThread %ld started", pthread_self());
    while (true) {
        pthread_cond_wait(proxy.current, &proxy.lock);
        if (proxy.pending) {
            proxy.current = proxy.pending;
            proxy.pending = NULL;
        }
        proxy.relocked = true;
        pthread_cond_signal(&stateCond);
    }
}

static void pthreadCondVarProxyListenOtherCondVar(pthread_cond_t* var) {
    pthread_mutex_lock(&proxy.lock);
    pthread_cond_broadcast(proxy.current);
    proxy.pending = var ?: &proxy.def;
    proxy.relocked = false;
    while(!proxy.relocked)
        pthread_cond_wait(&stateCond, &proxy.lock);
    pthread_mutex_unlock(&proxy.lock);
}
