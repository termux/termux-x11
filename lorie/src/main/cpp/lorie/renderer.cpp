#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "DanglingPointer"
#pragma ide diagnostic ignored "ConstantConditionsOC"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "UnreachableCode"
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#pragma ide diagnostic ignored "misc-no-recursion"
#pragma ide diagnostic ignored "readability-redundant-declaration"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#define __ANDROID_UNAVAILABLE_SYMBOLS_ARE_WEAK__

#define CVT_H_GRANULARITY 8

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <media/NdkImageReader.h>
#include <cmath>
#include <cstring>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <unistd.h>
#include "list.h"
#include "lorie.h"

// libEGL exports this only since API 26, weak so the library still loads below that.
__attribute__((weak)) EGLClientBuffer eglGetNativeClientBufferANDROID(const struct AHardwareBuffer* buffer);

#define log(...) __android_log_print(ANDROID_LOG_DEBUG, "gles-renderer", __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, "gles-renderer", __VA_ARGS__)

static constexpr EGLTimeKHR FENCE_TIMEOUT_NANOS = 100000000;

static void waitForFence(EGLDisplay egl_display, EGLSync fence, EGLint flags) {
    if (eglClientWaitSyncKHR(egl_display, fence, flags, FENCE_TIMEOUT_NANOS) == EGL_TIMEOUT_EXPIRED_KHR)
        loge("GPU fence wasn't signalled within %lldns, proceeding without it", (long long) FENCE_TIMEOUT_NANOS);
    eglDestroySyncKHR(egl_display, fence);
}

static GLuint createProgram(const char* p_vertex_source, const char* p_fragment_source);

static void* printEglError(const char* msg, int line) {
    char descBuf[32] = {0};
    char* desc;
    int err = eglGetError();
    switch(err) {
#define E(code, text) case code: desc = (char*) text; break
        case EGL_SUCCESS: desc = nullptr; // "No error"
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

    return nullptr;
}

static inline __always_inline void vprintEglError(const char* msg, int line) {
    printEglError(msg, line);
}

static void checkGlError(int line) {
    GLenum error;
    char *desc = nullptr;
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

void Renderer::bindTexture(GLuint id) const {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Renderer::reportViewport(int dstX, int dstY, int dstW, int dstH, float left, float top, float width, float height) {
    if (reportedViewportX == dstX && reportedViewportY == dstY &&
        reportedViewportW == dstW && reportedViewportH == dstH &&
        reportedSourceLeft == left && reportedSourceTop == top &&
        reportedSourceWidth == width && reportedSourceHeight == height)
        return;

    state->shown.x = reportedViewportX = dstX;
    state->shown.y = reportedViewportY = dstY;
    state->shown.width = reportedViewportW = dstW;
    state->shown.height = reportedViewportH = dstH;
    state->shown.sourceLeft = reportedSourceLeft = left;
    state->shown.sourceTop = reportedSourceTop = top;
    state->shown.sourceWidth = reportedSourceWidth = width;
    state->shown.sourceHeight = reportedSourceHeight = height;
    if (!__atomic_exchange_n(&state->shown.reported, 1, __ATOMIC_ACQ_REL))
        lorieSendShownChanged();
}

static const EGLint ctxattribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
};

// The window the context is kept current on while the real surface is gone. Nothing ever reads
// from the texture behind it, and neither it nor the surface is released, both live as long as
// the renderer does.
//
// Weird devices without proper EGL_KHR_surfaceless_context support
// We can not use pbuffer-based surfaces because it will require searching for configs supporting it
// and I am not sure all devices have configs supporting both pbuffers and regular surfaces simultaneously
static ANativeWindow* createDefaultWindow(JNIEnv* env) {
    if (__builtin_available(android 24, *)) {
        AImageReader* reader = nullptr; // Never released, lives as long as the renderer does, same as the window
        ANativeWindow* win = nullptr;
        if (AImageReader_new(1, 1, AIMAGE_FORMAT_RGBA_8888, 2, &reader) != AMEDIA_OK) {
            log("Failed to initialise ImageReader");
        } else {
            AImageReader_ImageListener listener = { .context = nullptr, .onImageAvailable = [](void* context, AImageReader* reader) {
                AImage* image = nullptr;
                if (AImageReader_acquireLatestImage(reader, &image) == AMEDIA_OK && image)
                    AImage_delete(image);
            } };
            if (AImageReader_setImageListener(reader, &listener) != AMEDIA_OK) {
                log("Failed to set ImageReader listener");
                AImageReader_delete(reader);
            } else if (AImageReader_getWindow(reader, &win) != AMEDIA_OK) {
                log("Failed to obtain ImageReader native window");
                AImageReader_delete(reader);
            }
        }

        if (win)
            return win;
    }

    jclass surfaceTextureClass = env->FindClass("android/graphics/SurfaceTexture");
    jclass surfaceClass = env->FindClass("android/view/Surface");
    if (!surfaceTextureClass || !surfaceClass) {
        log("Failed to find SurfaceTexture or Surface class");
        return nullptr;
    }

    jobject surfaceTexture = env->NewObject(surfaceTextureClass, env->GetMethodID(surfaceTextureClass, "<init>", "(Z)V"), true);
    jobject surface = surfaceTexture ? env->NewObject(surfaceClass, env->GetMethodID(surfaceClass, "<init>", "(Landroid/graphics/SurfaceTexture;)V"), surfaceTexture) : nullptr;
    if (!surface) {
        log("Failed to instantiate SurfaceTexture or Surface");
        env->ExceptionClear();
        return nullptr;
    }

    env->NewGlobalRef(surfaceTexture);
    env->NewGlobalRef(surface);
    return ANativeWindow_fromSurface(env, surface);
}

void* Renderer::initThread() {
    JavaVMAttachArgs attachArgs = { JNI_VERSION_1_6, "LorieRendererThread", nullptr };
    bool ready = jvm->AttachCurrentThread(&rendererEnv, &attachArgs) == JNI_OK;
    if (!ready)
        log("Failed to attach renderer thread to JVM");

    ready = ready && initGl();
    pthread_mutex_lock(&stateLock);
    legacyDrawingRequired = ready && needsLegacyDrawing();
    initDone = true;
    pthread_cond_broadcast(&stateChangeFinishCond);
    pthread_mutex_unlock(&stateLock);

    if (ready)
        threadLoop();
    return nullptr;
}

bool Renderer::initGl() {
    EGLint major, minor;
    EGLint numConfigs;
    EGLint *const alphaAttrib = &configAttribs[11];

    jclass surfaceClass = rendererEnv->FindClass("android/view/Surface");
    surfaceRelease = rendererEnv->GetMethodID(surfaceClass, "release", "()V");
    surfaceDestroy = rendererEnv->GetMethodID(surfaceClass, "destroy", "()V");
    rendererEnv->ExceptionClear();

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

    ctx = eglCreateContext(egl_display, cfg, nullptr, ctxattribs);
    if (ctx == EGL_NO_CONTEXT)
        return printEglError("eglCreateContext failed", __LINE__);

    win = defaultWin = createDefaultWindow(rendererEnv);
    if (!defaultWin)
        return printEglError("Got no window to keep the context current on", __LINE__);

    ANativeWindow_acquire(defaultWin);

    sfc = defaultSfc = eglCreateWindowSurface(egl_display, cfg, win, nullptr);

    if (eglMakeCurrent(egl_display, sfc, sfc, ctx) != EGL_TRUE)
        return printEglError("eglMakeCurrent failed", __LINE__);
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
    return true;
}

bool Renderer::init(JavaVM* vm, struct lorie_shared_server_state* serverState) {
    jvm = vm;
    state = serverState;

    pthread_mutex_init(&stateLock, nullptr);
    pthread_cond_init(&stateCond, nullptr);
    pthread_cond_init(&stateChangeFinishCond, nullptr);
    pthread_spin_init(&bufferLock, false);
    xorg_list_init(&addedBuffers);
    xorg_list_init(&buffers);
    xorg_list_init(&removedBuffers);

    pthread_create(&thread, nullptr, +[](void* cookie) -> void* {
        return ((Renderer*) cookie)->initThread();
    }, this);

    pthread_mutex_lock(&stateLock);
    while (!initDone)
        pthread_cond_wait(&stateChangeFinishCond, &stateLock);
    pthread_mutex_unlock(&stateLock);
    return legacyDrawingRequired;
}

bool Renderer::needsLegacyDrawing() {
    // Some devices sample an AHardwareBuffer written by the CPU with a different layout, here we are checking it.
    const EGLint imageAttributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
    EGLClientBuffer clientBuffer;
    EGLImageKHR img;
    bool legacyDrawing = false;
    AHardwareBuffer *new_ = nullptr;
    int status;
    AHardwareBuffer_Desc d0 = {
            .width = 64,
            .height = 64,
            .layers = 1,
            .format = AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM,
            .usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN
    };

    if (!__builtin_available(android 26, *)) {
        loge("No AHardwareBuffer on this platform, forcing legacy drawing");
        return true;
    }

    status = AHardwareBuffer_allocate(&d0, &new_);
    if (status != 0 || new_ == nullptr) {
        loge("Failed to allocate native buffer (%p, error %d)", new_, status);
        loge("Forcing legacy drawing");
        return true;
    }

    uint32_t *pixels;
    if (AHardwareBuffer_lock(new_, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr, (void **) &pixels) == 0) {
        pixels[0] = 0xAABBCCDD;
        AHardwareBuffer_unlock(new_, nullptr);
    } else {
        loge("Failed to lock native buffer (%p, error %d)", new_, status);
        loge("Forcing legacy drawing");
        AHardwareBuffer_release(new_);
        return true;
    }

    clientBuffer = eglGetNativeClientBufferANDROID(new_);
    if (!clientBuffer) {
        AHardwareBuffer_release(new_);
        printEglError("Failed to obtain EGLClientBuffer from AHardwareBuffer, forcing legacy drawing", __LINE__);
        return true;
    }

    if (!(img = eglCreateImageKHR(egl_display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, imageAttributes))) {
        loge("Failed to obtain EGLImageKHR from EGLClientBuffer");
        loge("Forcing legacy drawing");
        legacyDrawing = true;
    } else {
        // Reading the pixel back through the GPU must give what the CPU wrote, otherwise we fall back
        // to legacy drawing (uploading pixels via glTexSubImage2D).
        GLuint fbo = 0, texture = 0;
        glActiveTexture(GL_TEXTURE0); checkGlError();
        glGenTextures(1, &texture); checkGlError();
        bindTexture(texture);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, img); checkGlError();
        glGenFramebuffers(1, &fbo); checkGlError();
        glBindFramebuffer(GL_FRAMEBUFFER, fbo); checkGlError();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0); checkGlError();
        uint32_t pixel[64*64];
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel); checkGlError();
        if (pixel[0] != 0xAABBCCDD && pixel[0] != 0xFFBBCCDD) {
            log("Xlorie: GLES receives broken pixels. Forcing legacy drawing. 0x%X\n", pixel[0]);
            legacyDrawing = true;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &texture);
        eglDestroyImageKHR(egl_display, img);
    }

    AHardwareBuffer_release(new_);
    return legacyDrawing;
}

void Renderer::addBuffer(LorieBuffer* buf) {
    uint64_t id = LorieBuffer_description(buf)->id;
    pthread_spin_lock(&bufferLock);
    LorieBuffer* removed = LorieBufferList_findById(&removedBuffers, id);
    if (removed)
        LorieBuffer_addToList(removed, &buffers); // still attached to GL
    else if (!LorieBufferList_findById(&addedBuffers, id) && !LorieBufferList_findById(&buffers, id)) {
        LorieBuffer_acquire(buf);
        LorieBuffer_addToList(buf, &addedBuffers);
    }
    pthread_cond_signal(&stateCond);
    pthread_spin_unlock(&bufferLock);
}

void Renderer::removeBuffer(uint64_t id) {
    pthread_spin_lock(&bufferLock);
    LorieBuffer* buf = LorieBufferList_findById(&addedBuffers, id);
    if (buf) {
        LorieBuffer_removeFromList(buf);
        LorieBuffer_release(buf);
    } else {
        buf = LorieBufferList_findById(&buffers, id);
        if (buf) {
            // The buffer is attached to GL so we should release it from renderer thread.
            LorieBuffer_removeFromList(buf);
            LorieBuffer_addToList(buf, &removedBuffers);
        }
    }
    pthread_cond_signal(&stateCond);
    pthread_spin_unlock(&bufferLock);
}

void Renderer::setWindow(JNIEnv *env, jobject jsfc) {
    ANativeWindow* newWin = jsfc ? ANativeWindow_fromSurface(env, jsfc) : nullptr;

    pthread_mutex_lock(&stateLock);
    while(windowChanged)
        pthread_cond_wait(&stateChangeFinishCond, &stateLock);

    if (newWin && win == newWin) {
        ANativeWindow_release(newWin);
        pthread_mutex_unlock(&stateLock);
        return;
    }

    pendingWin = newWin;
    pendingSurface = newWin ? env->NewGlobalRef(jsfc) : nullptr;
    expectedW = expectedH = 0;
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

void Renderer::releaseWinAndSurface(ANativeWindow** anw, jobject* jsfc, EGLSurface *esfc) {
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

    if (jsfc && *jsfc) {
        rendererEnv->CallVoidMethod(*jsfc, surfaceRelease);
        if (surfaceDestroy)
            rendererEnv->CallVoidMethod(*jsfc, surfaceDestroy);
        rendererEnv->ExceptionClear();
        rendererEnv->DeleteGlobalRef(*jsfc);
        *jsfc = nullptr;
    }
}

void Renderer::applyView() {
    struct lorie_shared_server_state::lorie_view v = state->view;
    pthread_mutex_lock(&stateLock);
    viewportX = v.x;
    viewportY = v.y;
    viewportW = v.width;
    viewportH = v.height;
    expectedW = v.expectedWidth;
    expectedH = v.expectedHeight;
    hiddenBottom = v.hiddenBottom;
    zoomPercent = v.zoomPercent < 100 ? 100 : (v.zoomPercent > 400 ? 400 : v.zoomPercent);
    filtering = v.filtering;
    pinchAnchorSourceX = v.pinchAnchorSourceX;
    pinchAnchorSourceY = v.pinchAnchorSourceY;
    pinchAnchorFracX = v.pinchAnchorFracX;
    pinchAnchorFracY = v.pinchAnchorFracY;
    followCursorPan = v.followCursorPan;
    viewportChanged = true;
    reportedViewportX = reportedViewportY = reportedViewportW = reportedViewportH = -1;
    reportedSourceLeft = reportedSourceTop = reportedSourceWidth = reportedSourceHeight = -1.f;
    state->drawRequested = true;
    pthread_cond_signal(&stateCond);
    pthread_mutex_unlock(&stateLock);
}

void Renderer::refreshContext() {
    int width = pendingWin ? ANativeWindow_getWidth(pendingWin) : 0;
    int height = pendingWin ? ANativeWindow_getHeight(pendingWin) : 0;
    log("rendererSetWindow %p %d %d", pendingWin, width, height);

    releaseWinAndSurface(&win, &surface, &sfc);

    if (pendingWin && (width <= 0 || height <= 0)) {
        log("Xlorie: We've got invalid surface. Probably it became invalid before we started working with it.\n");
        releaseWinAndSurface(&pendingWin, &pendingSurface, nullptr);
    }

    win = pendingWin;
    surface = pendingSurface;
    pendingWin = nullptr;
    pendingSurface = nullptr;
    windowChanged = FALSE;

    if (!win) {
        win = defaultWin;
        eglMakeCurrent(egl_display, defaultSfc, defaultSfc, ctx);
        state->surfaceAvailable = false;
        return;
    }

    sfc = eglCreateWindowSurface(egl_display, cfg, win, nullptr);
    if (sfc == EGL_NO_SURFACE)
        return vprintEglError("eglCreateWindowSurface failed", __LINE__);

    if (eglMakeCurrent(egl_display, sfc, sfc, ctx) != EGL_TRUE) {
        state->surfaceAvailable = false;
        return vprintEglError("eglMakeCurrent failed", __LINE__);
    }

    eglSwapInterval(egl_display, 0);

    // We should redraw image at least once right after surface change
    state->surfaceAvailable = state->drawRequested = state->cursor.updated = win != defaultWin;

    glViewport(0, 0, ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));
    log("Xlorie: new surface applied: %p\n", sfc);
}

static void lockServerState(struct lorie_shared_server_state *state) {
    state->rendererWaiting = TRUE;
    pthread_mutex_lock(&state->lock);
    __atomic_store_n(&state->rendererWaiting, 0, __ATOMIC_RELEASE);
    syscall(SYS_futex, &state->rendererWaiting, FUTEX_WAKE, 1, nullptr, nullptr, 0);
}

// Keeps the shown region still while the cursor stays inside it, panning only when the cursor
// enters the 5% band near an edge.
static float panToCursor(float offset, float cursor, float shown, float total) {
    float edge = shown * 0.05f;
    if (cursor < offset + edge)
        offset = cursor - edge;
    else if (cursor > offset + shown - edge)
        offset = cursor - shown + edge;
    return fmaxf(0.f, fminf(offset, total - shown));
}

void Renderer::redrawLocked(bool* waitingForBuffers) {
    float xfactor = 1.f;
    const LorieBuffer_Desc *desc = nullptr;
    EGLSync fence;

    // The buffer will not be released until this function ends, but main thread can modify buffer list
    pthread_spin_lock(&bufferLock);
    LorieBuffer *buffer = LorieBufferList_findById(&buffers, state->rootWindowTextureID);
    // Probably X server requested us to draw removed buffer and immediately requested to remove it. Let's display it one last time.
    if (!buffer)
        buffer = LorieBufferList_findById(&removedBuffers, state->rootWindowTextureID);
    if (!buffer)
        *waitingForBuffers = true;
    pthread_spin_unlock(&bufferLock);
    if (!buffer) {
        log("Buffer %llu not found", state->rootWindowTextureID);
        return;
    }

    desc = LorieBuffer_description(buffer);

    int alignedExpectedW = expectedW - (expectedW % CVT_H_GRANULARITY);

    if (!expectedW || !expectedH || desc->height != expectedH ||
        (desc->width != alignedExpectedW && desc->width != expectedW)) {
        log("Buffer %llu is not of expected size, expecting %dx%d or %dx%d, got %dx%d",
            state->rootWindowTextureID, alignedExpectedW, expectedH, expectedW, expectedH,
            desc->width, desc->height);
        // Otherwise rendererShouldWait sees drawRequested or a pending cursor update and busy-spins
        // retrying this same mismatch instead of waiting for the buffer of the requested size.
        state->drawRequested = FALSE;
        *waitingForBuffers = true;
        return;
    }

    int surfaceH = ANativeWindow_getHeight(win);
    int surfaceW = ANativeWindow_getWidth(win);
    int renderViewportX = viewportX, renderViewportY = viewportY, renderViewportW = viewportW, renderViewportH = viewportH;
    float destinationScaleX = 1.f, destinationScaleY = 1.f;
    if (zoomPercent > 100 && viewportW > 0 && viewportH > 0) {
        float requestedScale = (float) zoomPercent / 100.f;
        float centerX = (float) viewportX + (float) viewportW / 2.f;
        float centerY = (float) viewportY + (float) viewportH / 2.f;
        float maxW = 2.f * fminf(centerX, (float) surfaceW - centerX);
        float maxH = 2.f * fminf(centerY, (float) surfaceH - centerY);
        destinationScaleX = fminf(requestedScale, maxW / (float) viewportW);
        destinationScaleY = fminf(requestedScale, maxH / (float) viewportH);
        if (destinationScaleX < 1.f)
            destinationScaleX = 1.f;
        if (destinationScaleY < 1.f)
            destinationScaleY = 1.f;

        renderViewportW = (int) lroundf((float) viewportW * destinationScaleX);
        renderViewportH = (int) lroundf((float) viewportH * destinationScaleY);
        renderViewportX = (int) lroundf(centerX - (float) renderViewportW / 2.f);
        renderViewportY = (int) lroundf(centerY - (float) renderViewportH / 2.f);
    }

    auto sourceWidth = (float) desc->width, sourceHeight = (float) desc->height;
    auto logicalSourceWidth = (float) expectedW, logicalSourceHeight = (float) expectedH;
    if (zoomPercent > 100) {
        float requestedScale = (float) zoomPercent / 100.f;
        sourceWidth = (float) expectedW * destinationScaleX / requestedScale;
        sourceHeight = (float) expectedH * destinationScaleY / requestedScale;
        logicalSourceWidth = sourceWidth;
        logicalSourceHeight = sourceHeight;
    }

    // The soft keyboard hides the bottom of the picture instead of the picture shrinking for it,
    // so only the part fitting above it is drawn, at the same scale as the whole picture.
    int cut = renderViewportY + renderViewportH - (viewportY + viewportH - hiddenBottom);
    bool bottomHidden = hiddenBottom > 0 && cut > 0 && cut < renderViewportH;
    if (bottomHidden) {
        float shown = (float) (renderViewportH - cut) / (float) renderViewportH;
        sourceHeight *= shown;
        logicalSourceHeight *= shown;
        renderViewportH -= cut;
    }

    // The keyboard swaps the zone the picture is at for the one it was left at on the other side of
    // the switch. Zoomed in the picture pans on its own while the keyboard is away, so there it is
    // only put back once the keyboard is gone, to where it was before the keyboard covered it.
    if (bottomHidden != bottomWasHidden) {
        bottomWasHidden = bottomHidden;
        float other = hiddenPanSourceTop;
        hiddenPanSourceTop = panSourceTop;
        if (other >= 0.f && (!bottomHidden || zoomPercent == 100))
            panSourceTop = other;
    }

    auto cursorX = (float) state->cursor.x, cursorY = (float) state->cursor.y; // snapshot, cursor.x/y is updated lock-free

    if (pinchAnchorSourceX >= 0.f) {
        panSourceLeft = fmaxf(0.f, fminf(pinchAnchorSourceX - pinchAnchorFracX * sourceWidth, (float) expectedW - sourceWidth));
        panSourceTop = fmaxf(0.f, fminf(pinchAnchorSourceY - pinchAnchorFracY * sourceHeight, (float) expectedH - sourceHeight));
    } else {
        // Cursor tracking while zoomed in is opt-out; a vertical shrink caused by the keyboard
        // hiding the bottom (rather than by zoom) always tracks the cursor regardless of that.
        bool trackCursor = followCursorPan || zoomPercent == 100;

        // The buffer can be a few pixels narrower than the screen because of the mode granularity,
        // so panning horizontally only makes sense when zoomed in.
        panSourceLeft = zoomPercent > 100 && trackCursor
                        ? panToCursor(panSourceLeft, cursorX, sourceWidth, (float) expectedW)
                        : (zoomPercent > 100 ? panSourceLeft : 0.f);
        panSourceTop = sourceHeight < (float) expectedH && trackCursor
                       ? panToCursor(panSourceTop, cursorY, sourceHeight, (float) expectedH)
                       : (sourceHeight < (float) expectedH ? panSourceTop : 0.f);
    }
    float sourceLeft = panSourceLeft, sourceTop = panSourceTop;

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, surfaceW, surfaceH);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(renderViewportX, surfaceH - renderViewportY - renderViewportH, renderViewportW, renderViewportH);

    reportViewport(renderViewportX, renderViewportY, renderViewportW, renderViewportH,
                   sourceLeft, sourceTop, logicalSourceWidth, logicalSourceHeight);

    // We should signal X server to not use root window while we actively copy it
    lockServerState(state);
    if (state->drawRequested)
        lorieSyncRootForGpu(desc->id);
    state->drawRequested = FALSE;

    LorieBuffer_bindTexture(buffer);
    if (desc->type == LORIEBUFFER_FD) {
        uploadDamagedRows(buffer);
        xfactor = (float) desc->width/(float) desc->stride;
    }
    drawRegion(0, -1.f, -1.f, 1.f, 1.f,
               sourceLeft / (float) desc->width * xfactor, sourceTop / (float) desc->height,
               (sourceLeft + sourceWidth) / (float) desc->width * xfactor,
               (sourceTop + sourceHeight) / (float) desc->height,
               LorieBuffer_isRgba(buffer));
    fence = eglCreateSyncKHR(egl_display, EGL_SYNC_FENCE_KHR, nullptr);
    glFlush();

    if (state->cursor.updated) {
        log("Xlorie: updating cursor\n");
        pthread_mutex_lock(&state->cursor.lock);
        state->cursor.updated = false;
        bindTexture(cursor.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei) state->cursor.width, (GLsizei) state->cursor.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, state->cursor.bits);
        pthread_mutex_unlock(&state->cursor.lock);
    }

    state->cursor.moved = FALSE;
    drawCursor(sourceWidth, sourceHeight, sourceLeft, sourceTop, cursorX, cursorY);
    glFlush();

    // Wait until root window drawing is finished before giving control back to X server
    waitForFence(egl_display, fence, 0);
    state->waitForNextFrame = true;
    pthread_mutex_unlock(&state->lock);

    if (eglSwapBuffers(egl_display, sfc) != EGL_TRUE)
        printEglError("Failed to swap buffers", __LINE__);

    // Perform a little drawing operation to make sure the next buffer is ready on the next invocation of drawing
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, 1, 1);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    fence = eglCreateSyncKHR(egl_display, EGL_SYNC_FENCE_KHR, nullptr);
    waitForFence(egl_display, fence, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR);

    state->renderedFrames++;
}

void Renderer::uploadDamagedRows(LorieBuffer* buffer) {
    const LorieBuffer_Desc *desc = LorieBuffer_description(buffer);
    bool all = uploadedBufferId != desc->id;
    int start = -1;

    uploadedBufferId = desc->id;
    for (int word = 0; word * 64 < desc->height; word++) {
        uint64_t rows = __atomic_exchange_n(&state->damagedRows[word], 0, __ATOMIC_ACQUIRE) | (all ? ~0ULL : 0);
        for (int bit = 0, y = word * 64; bit < 64 && y < desc->height; bit++, y++) {
            bool damaged = rows & (1ULL << bit);
            if (damaged && start < 0)
                start = y;
            else if (!damaged && start >= 0) {
                LorieBuffer_uploadRows(buffer, start, y);
                start = -1;
            }
        }
    }

    if (start >= 0)
        LorieBuffer_uploadRows(buffer, start, desc->height);
}

bool Renderer::shouldWait(bool *waitingForBuffers) {
    bool buffersChanged;
    if (viewportChanged) {
        // setWindow drops the expected size, so the buffer rejected right after it fits again
        // as soon as the viewport is reapplied, and no new buffer is going to arrive.
        viewportChanged = false;
        *waitingForBuffers = false;
    }
    pthread_spin_lock(&bufferLock);
    buffersChanged = !xorg_list_is_empty(&addedBuffers) || !xorg_list_is_empty(&removedBuffers);
    pthread_spin_unlock(&bufferLock);
    if (windowChanged || buffersChanged)
        // If there are pending changes we should process them immediately.
        return false;

    if (lastRequestedBufferId != state->rootWindowTextureID)
        *waitingForBuffers = false;
    lastRequestedBufferId = state->rootWindowTextureID;

    if (!state->surfaceAvailable || state->waitForNextFrame || *waitingForBuffers)
        // Even in the case if there are pending changes, we can not draw it without rendering surface
        return true;

    if (state->drawRequested || state->cursor.moved || state->cursor.updated)
        // X server reported drawing or cursor changes, no need to wait.
        return false;

    // Probably spurious wake, no changes we can work with.
    return true;
}

void Renderer::threadLoop() {
    LorieBuffer* buf;
    bool waitingForBuffers = false;
    pthread_mutex_lock(&stateLock);
    while (true) {
        while (shouldWait(&waitingForBuffers))
            pthread_cond_wait(&stateCond, &stateLock);

        if (windowChanged)
            refreshContext();

        // Attach all pending buffers to GL.
        pthread_spin_lock(&bufferLock);
        while((buf = LorieBufferList_first(&addedBuffers))) {
            LorieBuffer_attachToGL(buf);
            LorieBuffer_addToList(buf, &buffers);
            waitingForBuffers = false;
        }
        pthread_spin_unlock(&bufferLock);

        pthread_cond_broadcast(&stateChangeFinishCond);
        pthread_mutex_unlock(&stateLock);

        if (state->surfaceAvailable && !state->waitForNextFrame &&
            (state->drawRequested || state->cursor.moved || state->cursor.updated))
            redrawLocked(&waitingForBuffers);

        pthread_spin_lock(&bufferLock);
        while((buf = LorieBufferList_first(&removedBuffers))) {
            LorieBuffer_removeFromList(buf);
            LorieBuffer_detachFromGL(buf);
            LorieBuffer_release(buf);
        }
        pthread_spin_unlock(&bufferLock);
        pthread_mutex_lock(&stateLock);
    }
}

static GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLint compiled = 0, infoLen = 0;
    GLuint shader = glCreateShader(shaderType);
    if (!shader)
        return 0;

    glShaderSource(shader, 1, &pSource, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled)
        return shader;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen) {
        char buf[infoLen];
        glGetShaderInfoLog(shader, infoLen, nullptr, buf);
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
        glGetProgramInfoLog(program, bufLength, nullptr, buf);
        log("renderer: Could not link program:\n%s\n", buf);
    }
    glDeleteProgram(program);

    return 0;
}

void Renderer::drawRegion(GLuint id, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, uint8_t flip) {
    float coords[16] = {
        x0, -y0, u0, v0,
        x1, -y0, u1, v0,
        x0, -y1, u0, v1,
        x1, -y1, u1, v1,
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

void Renderer::drawCursor(float displayWidth, float displayHeight, float sourceLeft, float sourceTop, float cursorX, float cursorY) {
    float x, y, w, h;

    if (!state->cursor.width || !state->cursor.height || !state->cursor.visible)
        return;

    x = 2.f * (cursorX - sourceLeft - (float) state->cursor.xhot) / displayWidth - 1.f;
    y = 2.f * (cursorY - sourceTop - (float) state->cursor.yhot) / displayHeight - 1.f;
    w = 2.f * (float) state->cursor.width / displayWidth;
    h = 2.f * (float) state->cursor.height / displayHeight;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawRegion(cursor.id, x, y, x + w, y + h, 0.f, 0.f, 1.f, 1.f, false);
    glDisable(GL_BLEND);
}
