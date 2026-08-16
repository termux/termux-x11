#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "DanglingPointer"
#pragma ide diagnostic ignored "ConstantConditionsOC"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "UnreachableCode"
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#pragma ide diagnostic ignored "misc-no-recursion"
#pragma ide diagnostic ignored "readability-redundant-declaration"
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
#include <dlfcn.h>
#include <cmath>
#include <cstring>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#include "list.h"
#include "lorie.h"

// libEGL exports this only since API 26, weak so the library still loads below that.
__attribute__((weak)) EGLClientBuffer eglGetNativeClientBufferANDROID(const struct AHardwareBuffer* buffer);

#define log(...) __android_log_print(ANDROID_LOG_DEBUG, "gles-renderer", __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, "gles-renderer", __VA_ARGS__)

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

// Notifies activity.cpp's end of the GUI<->X server socket immediately when a GPU copy batch
// finishes, instead of it waiting for the next vblank-tick poll.
void Renderer::notifyGpuCopyDone() {
    if (connFdPtr && *connFdPtr != -1) {
        lorieEvent e = { .type = EVENT_GPU_COPY_DONE };
        write(*connFdPtr, &e, sizeof(e));
    }
}

void Renderer::bindTexture(GLuint id) const {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Renderer::reportViewport(int dstX, int dstY, int dstW, int dstH, float left, float top, float width, float height) {
    JNIEnv* env = rendererEnv;
    if (!env || !lorieViewClass || !setRendererViewportMethod)
        return;

    if (reportedViewportX == dstX && reportedViewportY == dstY &&
        reportedViewportW == dstW && reportedViewportH == dstH &&
        reportedSourceLeft == left && reportedSourceTop == top &&
        reportedSourceWidth == width && reportedSourceHeight == height)
        return;

    env->CallStaticVoidMethod(lorieViewClass, setRendererViewportMethod, dstX, dstY, dstW, dstH, left, top, width, height);
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    } else {
        reportedViewportX = dstX;
        reportedViewportY = dstY;
        reportedViewportW = dstW;
        reportedViewportH = dstH;
        reportedSourceLeft = left;
        reportedSourceTop = top;
        reportedSourceWidth = width;
        reportedSourceHeight = height;
    }
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
    if (jvm->AttachCurrentThread(&rendererEnv, nullptr) != JNI_OK) {
        log("Failed to attach renderer thread to JVM");
        return nullptr;
    }

    EGLint major, minor;
    EGLint numConfigs;
    EGLint *const alphaAttrib = &configAttribs[11];

    pthread_setname_np(pthread_self(), "LorieRendererThread");

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

    ctx = eglCreateContext(egl_display, cfg, nullptr, ctxattribs);
    if (ctx == EGL_NO_CONTEXT)
        return printEglError("eglCreateContext failed", __LINE__);

    win = defaultWin = createDefaultWindow(rendererEnv);
    if (!defaultWin)
        return printEglError("Got no window to keep the context current on", __LINE__);

    ANativeWindow_acquire(defaultWin);

    sfc = defaultSfc = eglCreateWindowSurface(egl_display, cfg, win, nullptr);

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

    threadLoop();
    return nullptr;
}

void Renderer::init(JNIEnv* env) {
    if (ctx)
        return;

    env->GetJavaVM(&jvm);
    jclass clazz = env->FindClass("com/termux/x11/LorieView");
    lorieViewClass = (jclass) env->NewGlobalRef(clazz);
    setRendererViewportMethod = env->GetStaticMethodID(lorieViewClass, "setRendererViewport", "(IIIIFFFF)V");

    pthread_mutex_init(&stateLock, nullptr);

    // Created once, never recreated; only the fd is (re)sent to the X server whenever it (re)connects.
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    stateCondFd = LorieBuffer_createRegion("renderer-cond", sizeof(pthread_cond_t));
    stateCond = stateCondFd == -1 ? (pthread_cond_t*) MAP_FAILED : (pthread_cond_t*) mmap(nullptr, sizeof(pthread_cond_t), PROT_READ|PROT_WRITE, MAP_SHARED, stateCondFd, 0);
    if (stateCond == MAP_FAILED) {
        loge("Failed to allocate renderer wakeup cond var, aborting");
        abort();
    }
    pthread_cond_init(stateCond, &cond_attr);

    pthread_cond_init(&stateChangeFinishCond, nullptr);
    pthread_spin_init(&bufferLock, false);

    stopping = false;
    pthread_create(&thread, nullptr, +[](void* cookie) -> void* {
        return ((Renderer*) cookie)->initThread();
    }, this);
}

void Renderer::destroy() {
    if (!ctx)
        return; // never initialized, or already destroyed

    pthread_mutex_lock(&stateLock);
    stopping = true;
    pthread_cond_signal(stateCond);
    pthread_mutex_unlock(&stateLock);

    pthread_join(thread, nullptr);
    thread = 0;
    ctx = EGL_NO_CONTEXT; // re-passes init()'s `if (ctx) return;` guard for a future re-init
}

int Renderer::getWakeupCondFd() const {
    return stateCondFd;
}

void Renderer::setFiltering(jint f) {
    filtering = f;
}

void Renderer::testCapabilities(int* legacy_drawing, int* gpu_present_disabled) {
    // Some devices do not support sampling from HAL_PIXEL_FORMAT_BGRA_8888, here we are checking it.
    const EGLint imageAttributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
    EGLint numConfigs;
    EGLClientBuffer clientBuffer;
    EGLImageKHR img;
    EGLint major, minor;
    AHardwareBuffer *new_ = nullptr;
    int status;
    AHardwareBuffer_Desc d0 = {
            .width = 64,
            .height = 64,
            .layers = 1,
            .usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            .format = AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM
    };

    if (!__builtin_available(android 26, *)) {
        loge("No AHardwareBuffer on this platform, forcing legacy drawing");
        *legacy_drawing = 1;
        return;
    }

    if (egl_display == EGL_NO_DISPLAY) {
        egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (egl_display == EGL_NO_DISPLAY)
            return vprintEglError("Got no EGL display", __LINE__);
    }

    if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE)
        return vprintEglError("Unable to initialize EGL", __LINE__);

    loge("Xlorie: Initialized EGL version %d.%d\n", major, minor);
    eglBindAPI(EGL_OPENGL_ES_API);

    status = AHardwareBuffer_allocate(&d0, &new_);
    if (status != 0 || new_ == nullptr) {
        loge("Failed to allocate native buffer (%p, error %d)", new_, status);
        loge("Forcing legacy drawing");
        *legacy_drawing = 1;
        return;
    }

    uint32_t *pixels;
    if (AHardwareBuffer_lock(new_, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr, (void **) &pixels) == 0) {
        pixels[0] = 0xAABBCCDD;
        AHardwareBuffer_unlock(new_, nullptr);
    } else {
        loge("Failed to lock native buffer (%p, error %d)", new_, status);
        loge("Forcing legacy drawing");
        *legacy_drawing = 1;
        AHardwareBuffer_release(new_);
        return;
    }

    // Cross-process AHardwareBuffer sync relies on dma-buf. Send the buffer's handle through a
    // local socketpair exactly as it would be sent to the X server, and check whether the fd(s)
    // that come out are real dma-bufs. Skip if Present's GPU-copy offload is already off, since
    // that's the only thing this depends on.
    if (!*gpu_present_disabled) {
        int sv[2] = { -1, -1 };
        bool dmaBufFound = false;
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0) {
            AHardwareBuffer_sendHandleToUnixSocket(new_, sv[0]);
            shutdown(sv[0], SHUT_WR);
            for (;;) {
                uint8_t data[4096];
                union {
                    uint8_t buf[CMSG_SPACE(32 * sizeof(int))];
                    struct cmsghdr align;
                } control;
                struct iovec iov = { .iov_base = data, .iov_len = sizeof(data) };
                struct msghdr msg = {
                    .msg_iov = &iov, .msg_iovlen = 1,
                    .msg_control = control.buf, .msg_controllen = sizeof(control.buf),
                };
                ssize_t n = recvmsg(sv[1], &msg, 0);
                if (n <= 0)
                    break;
                for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
                    if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)
                        continue;
                    int *fds = (int *) CMSG_DATA(cmsg);
                    int nfds = (int) ((cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int));
                    for (int i = 0; i < nfds; i++) {
                        char path[32], target[256];
                        snprintf(path, sizeof(path), "/proc/self/fd/%d", fds[i]);
                        ssize_t len = readlink(path, target, sizeof(target) - 1);
                        if (len > 0) {
                            target[len] = '\0';
                            if (strstr(target, "dmabuf") || strstr(target, "dma_heap") || strstr(target, "/dev/dma"))
                                dmaBufFound = true;
                        }
                        close(fds[i]);
                    }
                }
            }
            close(sv[0]);
            close(sv[1]);
        } else {
            dmaBufFound = true; // Can't check, assume the best rather than force a slower fallback.
        }

        if (!dmaBufFound) {
            loge("AHardwareBuffer is not dma-buf-backed on this device, disabling Present GPU offload");
            *gpu_present_disabled = 1;
        }
    }

    clientBuffer = eglGetNativeClientBufferANDROID(new_);
    if (!clientBuffer) {
        *legacy_drawing = 1;
        AHardwareBuffer_release(new_);
        return vprintEglError("Failed to obtain EGLClientBuffer from AHardwareBuffer, forcing legacy drawing", __LINE__);
    }

    if (!(img = eglCreateImageKHR(egl_display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, imageAttributes))) {
        loge("Failed to obtain EGLImageKHR from EGLClientBuffer");
        loge("Forcing legacy drawing");
        *legacy_drawing = 1;
        AHardwareBuffer_release(new_);
    } else {
        // For some reason all devices I checked had no GL_EXT_texture_format_BGRA8888 support, but some of them still provided BGRA extension.
        // EGL does not provide functions to query texture format in runtime.
        // Workarounds are less performant but at least they let us use Termux:X11 on devices with missing BGRA support.
        // We handle two cases.
        // If resulting texture has BGRA format but still drawing RGBA we should flip format to RGBA and flip pixels manually in shader.
        // In the case if for some reason we can not use HAL_PIXEL_FORMAT_BGRA_8888 we should fallback to legacy drawing method (uploading pixels via glTexImage2D).
        configAttribs[1] = EGL_PBUFFER_BIT;
        EGLConfig checkcfg = nullptr;
        GLuint fbo = 0, texture = 0;
        if (eglChooseConfig(egl_display, configAttribs, &checkcfg, 1, &numConfigs) != EGL_TRUE)
            return vprintEglError("check eglChooseConfig failed", __LINE__);

        EGLContext testctx = eglCreateContext(egl_display, checkcfg, nullptr, ctxattribs);
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

        // Intel's Mesa driver has a race that makes dma-buf cross-process write visibility
        // unreliable, so blacklist it outright.
        if (!*gpu_present_disabled) {
            const char *renderer = (const char *) glGetString(GL_RENDERER);
            if (renderer && strstr(renderer, "Mesa") && strstr(renderer, "Intel")) {
                loge("Detected Intel Mesa driver (GL_RENDERER=%s), disabling Present GPU offload", renderer);
                *gpu_present_disabled = 1;
            }
        }

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
            *legacy_drawing = 1;
        }
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(egl_display, testctx);
        eglDestroyImageKHR(egl_display, img);
        eglDestroySurface(egl_display, checksfc);
        AHardwareBuffer_release(new_);
    }
}

void rendererTestCapabilities(int* legacy_drawing, int* gpu_present_disabled) {
    Renderer scratch;
    scratch.testCapabilities(legacy_drawing, gpu_present_disabled);
}

void Renderer::setSharedState(struct lorie_shared_server_state* newState) {
    pthread_mutex_lock(&stateLock);
    pendingState = newState;
    stateChanged = true;
    pthread_cond_signal(stateCond);

    while(stateChanged)
        pthread_cond_wait(&stateChangeFinishCond, &stateLock);

    pthread_mutex_unlock(&stateLock);
}

void Renderer::addBuffer(LorieBuffer* buf) {
    pthread_spin_lock(&bufferLock);
    LorieBuffer_addToList(buf, &addedBuffers);
    pthread_cond_signal(stateCond);
    pthread_spin_unlock(&bufferLock);
}

void Renderer::removeBuffer(uint64_t id) {
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

void Renderer::removeAllBuffers() {
    LorieBuffer *buf = nullptr;

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

void Renderer::setWindow(JNIEnv *env, jobject jsfc) {
    ANativeWindow* newWin = jsfc ? ANativeWindow_fromSurface(env, jsfc) : nullptr;
    if (newWin)
        ANativeWindow_acquire(newWin);

    pthread_mutex_lock(&stateLock);
    if (newWin && pendingWin == newWin) {
        ANativeWindow_release(newWin);
        pthread_mutex_unlock(&stateLock);
        return;
    }

    if (pendingWin)
        ANativeWindow_release(pendingWin);

    pendingWin = newWin;
    expectedW = expectedH = 0;
    windowChanged = TRUE;

    pthread_cond_signal(stateCond);

    // We should wait until renderer destroys EGLSurface before SurfaceCallback::surfaceDestroyed finishes
    // Otherwise we will have weird errors like
    // `freeAllBuffers: 1 buffers were freed while being dequeued!`
    // or
    // `query: BufferQueue has been abandoned`
    while(windowChanged)
        pthread_cond_wait(&stateChangeFinishCond, &stateLock);

    pthread_mutex_unlock(&stateLock);
}

void Renderer::releaseWinAndSurface(ANativeWindow** anw, EGLSurface *esfc) {
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

void Renderer::setViewport(int x, int y, int w, int h, int ew, int eh, int hidden) {
    pthread_mutex_lock(&stateLock);
    viewportX = x;
    viewportY = y;
    viewportW = w;
    viewportH = h;
    expectedW = ew;
    expectedH = eh;
    hiddenBottom = hidden;
    viewportChanged = true;
    reportedViewportX = reportedViewportY = reportedViewportW = reportedViewportH = -1;
    reportedSourceLeft = reportedSourceTop = reportedSourceWidth = reportedSourceHeight = -1.f;
    if (state)
        state->drawRequested = true;
    pthread_cond_signal(stateCond);
    pthread_mutex_unlock(&stateLock);
}

void Renderer::setZoom(int percent) {
    pthread_mutex_lock(&stateLock);
    zoomPercent = percent < 100 ? 100 : (percent > 400 ? 400 : percent);
    reportedViewportX = reportedViewportY = reportedViewportW = reportedViewportH = -1;
    reportedSourceLeft = reportedSourceTop = reportedSourceWidth = reportedSourceHeight = -1.f;
    if (state)
        state->drawRequested = true;
    pthread_cond_signal(stateCond);
    pthread_mutex_unlock(&stateLock);
}

void Renderer::refreshContext() {
    int width = pendingWin ? ANativeWindow_getWidth(pendingWin) : 0;
    int height = pendingWin ? ANativeWindow_getHeight(pendingWin) : 0;
    log("rendererSetWindow %p %d %d", pendingWin, width, height);

    releaseWinAndSurface(&win, &sfc);

    if (pendingWin && (width <= 0 || height <= 0)) {
        log("Xlorie: We've got invalid surface. Probably it became invalid before we started working with it.\n");
        releaseWinAndSurface(&pendingWin, nullptr);
    }

    win = pendingWin;
    pendingWin = nullptr;
    windowChanged = FALSE;

    if (!win) {
        win = defaultWin;
        eglMakeCurrent(egl_display, defaultSfc, defaultSfc, ctx);
        if (state)
            state->surfaceAvailable = false;
        notifyGpuCopyDone(); // Wake up any GPU copy stuck waiting on a surface we no longer have.
        return;
    }

    sfc = eglCreateWindowSurface(egl_display, cfg, win, nullptr);
    if (sfc == EGL_NO_SURFACE)
        return vprintEglError("eglCreateWindowSurface failed", __LINE__);

    if (eglMakeCurrent(egl_display, sfc, sfc, ctx) != EGL_TRUE) {
        if (state)
            state->surfaceAvailable = false;
        notifyGpuCopyDone();
        return vprintEglError("eglMakeCurrent failed", __LINE__);
    }

    eglSwapInterval(egl_display, 0);

    // We should redraw image at least once right after surface change
    if (state)
        state->surfaceAvailable = state->drawRequested = state->cursor.updated = win != defaultWin;

    glViewport(0, 0, ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));
    log("Xlorie: new surface applied: %p\n", sfc);
}

// Drains the deferred GPU copy queue (filled by present_execute_copy) into the root texture via
// an FBO. Assumes the caller holds state->lock and will flush/fence before unlocking - returns
// the highest drained serial WITHOUT publishing it to completedSerial, since the caller must only
// do that after the fence confirms the GPU actually finished (not just submitted) the draws;
// publishing early would let the client's next write race our still-in-flight read.
// Looks up a registered buffer by id, waiting briefly (bounded) if it hasn't arrived over the
// async registration socket yet instead of busy-spinning the outer loop.
LorieBuffer *Renderer::findBufferWithRetry(uint64_t id) {
    LorieBuffer *buf;
    int attempt;

    pthread_spin_lock(&bufferLock);
    buf = LorieBufferList_findById(&buffers, id);
    if (!buf && (buf = LorieBufferList_findById(&addedBuffers, id))) {
        LorieBuffer_attachToGL(buf);
        LorieBuffer_addToList(buf, &buffers);
    }
    pthread_spin_unlock(&bufferLock);

    for (attempt = 0; attempt < 20 && !buf; attempt++) {
        usleep(5000);
        pthread_spin_lock(&bufferLock);
        buf = LorieBufferList_findById(&buffers, id);
        if (!buf && (buf = LorieBufferList_findById(&addedBuffers, id))) {
            LorieBuffer_attachToGL(buf);
            LorieBuffer_addToList(buf, &buffers);
        }
        pthread_spin_unlock(&bufferLock);
    }
    return buf;
}

uint64_t Renderer::applyPendingGpuCopiesLocked() {
    bool fboSetUp = false;
    uint64_t lastSerial = 0;
    uint64_t boundDstId = 0;
    GLint prevViewport[4];

    if (!state || state->gpuCopyQueue.readIndex == state->gpuCopyQueue.writeIndex)
        return 0;

    while (state->gpuCopyQueue.readIndex != state->gpuCopyQueue.writeIndex) {
        LorieGpuCopyEntry entry = state->gpuCopyQueue.entries[state->gpuCopyQueue.readIndex % LORIE_GPU_COPY_QUEUE_CAPACITY];
        LorieBuffer *src = findBufferWithRetry(entry.srcBufferId);
        LorieBuffer *dst = findBufferWithRetry(entry.dstBufferId);

        if (!src)
            log("rendererApplyPendingGpuCopies: source buffer %llu not found after waiting, skipping\n", (unsigned long long) entry.srcBufferId);
        if (!dst)
            log("rendererApplyPendingGpuCopies: destination buffer %llu not found after waiting, skipping\n", (unsigned long long) entry.dstBufferId);

        if (src && dst) {
            const LorieBuffer_Desc *srcDesc = LorieBuffer_description(src);
            const LorieBuffer_Desc *dstDesc = LorieBuffer_description(dst);
            int i;

            if (!fboSetUp) {
                glGetIntegerv(GL_VIEWPORT, prevViewport);
                if (!gpuCopyFbo)
                    glGenFramebuffers(1, &gpuCopyFbo);
                glBindFramebuffer(GL_FRAMEBUFFER, gpuCopyFbo);
                fboSetUp = true;
            }
            // Different entries can target different pixmaps (root, or a Composite-redirected
            // window's own backing pixmap); only rebind the FBO's attachment when it changes.
            if (boundDstId != entry.dstBufferId) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, LorieBuffer_getGLTextureId(dst), 0);
                glViewport(0, 0, dstDesc->width, dstDesc->height);
                boundDstId = entry.dstBufferId;

                // Diagnostic: GLES2 has no glGetTexLevelParameteriv, so ask the AHardwareBuffer
                // itself what it was actually allocated as, instead of trusting our own desc.
                {
                    if (lorieDebugEnabled && (dstSizeLogCount++ & 15) == 0 && dstDesc->buffer) {
                        AHardwareBuffer_Desc realDstDesc;
                        AHardwareBuffer_describe(dstDesc->buffer, &realDstDesc);
                        loge("gpucopy dst texId=%u real AHB size %ux%u stride=%u vs LorieBuffer desc %dx%d\n",
                             LorieBuffer_getGLTextureId(dst), realDstDesc.width, realDstDesc.height,
                             realDstDesc.stride, dstDesc->width, dstDesc->height);
                    }
                }
            }

            LorieBuffer_bindTexture(src);
            {
                if (lorieDebugEnabled && (srcSizeLogCount++ & 15) == 0 && srcDesc->buffer) {
                    AHardwareBuffer_Desc realSrcDesc;
                    AHardwareBuffer_describe(srcDesc->buffer, &realSrcDesc);
                    loge("gpucopy src texId=%u real AHB size %ux%u stride=%u vs LorieBuffer desc %dx%d (stride=%d)\n",
                         LorieBuffer_getGLTextureId(src), realSrcDesc.width, realSrcDesc.height,
                         realSrcDesc.stride, srcDesc->width, srcDesc->height, srcDesc->stride);
                }
            }
            for (i = 0; i < entry.numRects; i++) {
                LorieGpuCopyRect r = entry.rects[i];
                float x0 = 2.f * (float) (r.x1 + entry.xOff) / (float) dstDesc->width - 1.f;
                float x1 = 2.f * (float) (r.x2 + entry.xOff) / (float) dstDesc->width - 1.f;
                // FBO writes and on-screen draws use opposite y conventions here, unlike x.
                float y0 = 1.f - 2.f * (float) (r.y1 + entry.yOff) / (float) dstDesc->height;
                float y1 = 1.f - 2.f * (float) (r.y2 + entry.yOff) / (float) dstDesc->height;
                // EGLImage-backed textures sample by logical width regardless of row stride;
                // only our own CPU-uploaded LORIEBUFFER_FD texture is stride-wide.
                float srcUvDivisor = srcDesc->type == LORIEBUFFER_FD ? (float) srcDesc->stride : (float) srcDesc->width;
                float u0 = (float) r.x1 / srcUvDivisor;
                float u1 = (float) r.x2 / srcUvDivisor;
                float v0 = (float) r.y1 / (float) srcDesc->height;
                float v1 = (float) r.y2 / (float) srcDesc->height;
                // Only swap channels if src/dst storage formats actually differ.
                uint8_t needsSwizzle = LorieBuffer_isRgba(src) != LorieBuffer_isRgba(dst);
                log("rendererApplyPendingGpuCopies: rect (%d,%d)-(%d,%d) off=(%d,%d) -> ndc=(%.3f,%.3f)-(%.3f,%.3f) uv=(%.3f,%.3f)-(%.3f,%.3f) srcTex=%u dstTex=%u swizzle=%d\n",
                    r.x1, r.y1, r.x2, r.y2, entry.xOff, entry.yOff, x0, y0, x1, y1, u0, v0, u1, v1,
                    LorieBuffer_getGLTextureId(src), LorieBuffer_getGLTextureId(dst), needsSwizzle);
                drawRegion(0, x0, y0, x1, y1, u0, v0, u1, v1, needsSwizzle);
            }
        }

        lastSerial = entry.serial;
        state->gpuCopyQueue.readIndex++;
    }

    if (fboSetUp) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    }
    return lastSerial;
}

// Standalone entry point used by the renderer thread's main loop. Used when no redraw is going to
// happen on this tick (rare for GPU copies in practice, since scheduling one also marks damage
// non-empty - see lorieTryScheduleGpuCopy), so it has to take the lock and fence/unlock itself.
void Renderer::applyPendingGpuCopies() {
    uint64_t serial;
    if (!state || state->gpuCopyQueue.readIndex == state->gpuCopyQueue.writeIndex)
        return;
    lorie_mutex_lock(&state->lock, &state->lockingPid);
    serial = applyPendingGpuCopiesLocked();
    if (serial) {
        EGLSync fence = eglCreateSyncKHR(egl_display, EGL_SYNC_FENCE_KHR, nullptr);
        glFlush();
        eglClientWaitSyncKHR(egl_display, fence, 0, EGL_FOREVER);
        eglDestroySyncKHR(egl_display, fence);
        // Only now that the GPU has actually finished (not just been told to start) is it safe to
        // let present_execute_copy release/idle the source pixmap back to the client.
        state->gpuCopyQueue.completedSerial = serial;
        notifyGpuCopyDone();
    }
    lorie_mutex_unlock(&state->lock, &state->lockingPid);
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

    // Early returns below skip the applyPendingGpuCopiesLocked() call further down, which would
    // stall copies queued for windows unrelated to root while root itself isn't ready yet.
    if (state->gpuCopyQueue.readIndex != state->gpuCopyQueue.writeIndex)
        applyPendingGpuCopies();

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

        renderViewportW = (int) ((float) viewportW * destinationScaleX + 0.5f);
        renderViewportH = (int) ((float) viewportH * destinationScaleY + 0.5f);
        renderViewportX = (int) (centerX - (float) renderViewportW / 2.f + 0.5f);
        renderViewportY = (int) (centerY - (float) renderViewportH / 2.f + 0.5f);
    }

    float sourceWidth = (float) desc->width, sourceHeight = (float) desc->height;
    float logicalSourceWidth = (float) expectedW, logicalSourceHeight = (float) expectedH;
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

    // The buffer can be a few pixels narrower than the screen because of the mode granularity,
    // so panning horizontally only makes sense when zoomed in.
    panSourceLeft = zoomPercent > 100
                    ? panToCursor(panSourceLeft, (float) state->cursor.x, sourceWidth, (float) expectedW) : 0.f;
    panSourceTop = sourceHeight < (float) expectedH
                   ? panToCursor(panSourceTop, (float) state->cursor.y, sourceHeight, (float) expectedH) : 0.f;
    float sourceLeft = panSourceLeft, sourceTop = panSourceTop;

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, surfaceW, surfaceH);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(renderViewportX, surfaceH - renderViewportY - renderViewportH, renderViewportW, renderViewportH);

    reportViewport(renderViewportX, renderViewportY, renderViewportW, renderViewportH,
                   sourceLeft, sourceTop, logicalSourceWidth, logicalSourceHeight);

    // We should signal X server to not use root window while we actively copy it
    lorie_mutex_lock(&state->lock, &state->lockingPid);
    // Share this draw's flush+fence below instead of a separate round trip per frame.
    uint64_t gpuCopySerial = applyPendingGpuCopiesLocked();
    state->drawRequested = FALSE;

    LorieBuffer_bindTexture(buffer);
    if (desc->type == LORIEBUFFER_FD)
        xfactor = (float) desc->width/(float) desc->stride;
    drawRegion(0, -1.f, -1.f, 1.f, 1.f,
               sourceLeft / (float) desc->width * xfactor, sourceTop / (float) desc->height,
               (sourceLeft + sourceWidth) / (float) desc->width * xfactor,
               (sourceTop + sourceHeight) / (float) desc->height,
               LorieBuffer_isRgba(buffer));
    fence = eglCreateSyncKHR(egl_display, EGL_SYNC_FENCE_KHR, nullptr);
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
    drawCursor(sourceWidth, sourceHeight, sourceLeft, sourceTop);
    glFlush();

    // Wait until root window drawing is finished before giving control back to X server
    eglClientWaitSyncKHR(egl_display, fence, 0, EGL_FOREVER);
    eglDestroySyncKHR(egl_display, fence);
    if (gpuCopySerial) {
        state->gpuCopyQueue.completedSerial = gpuCopySerial;
        notifyGpuCopyDone();
    }
    state->waitForNextFrame = true;
    lorie_mutex_unlock(&state->lock, &state->lockingPid);

    if (eglSwapBuffers(egl_display, sfc) != EGL_TRUE)
        printEglError("Failed to swap buffers", __LINE__);

    // Perform a little drawing operation to make sure the next buffer is ready on the next invocation of drawing
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, 1, 1);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    fence = eglCreateSyncKHR(egl_display, EGL_SYNC_FENCE_KHR, nullptr);
    eglClientWaitSyncKHR(egl_display, fence, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, EGL_FOREVER);
    eglDestroySyncKHR(egl_display, fence);

    state->renderedFrames++;
}

bool Renderer::shouldWait(bool *waitingForBuffers) {
    bool buffersChanged, gpuCopyPending;
    if (viewportChanged) {
        // setWindow drops the expected size, so the buffer rejected right after it fits again
        // as soon as the viewport is reapplied, and no new buffer is going to arrive.
        viewportChanged = false;
        *waitingForBuffers = false;
    }
    pthread_spin_lock(&bufferLock);
    buffersChanged = !xorg_list_is_empty(&addedBuffers) || !xorg_list_is_empty(&removedBuffers);
    pthread_spin_unlock(&bufferLock);
    gpuCopyPending = state && state->gpuCopyQueue.readIndex != state->gpuCopyQueue.writeIndex;
    if (stateChanged || windowChanged || buffersChanged || gpuCopyPending)
        // If there are pending changes we should process them immediately.
        return false;

    if (state) {
        if (lastRequestedBufferId != state->rootWindowTextureID)
            *waitingForBuffers = false;
        lastRequestedBufferId = state->rootWindowTextureID;
    }

    if (!state || !state->surfaceAvailable || state->waitForNextFrame || *waitingForBuffers)
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
    while (!stopping) {
        while (!stopping && shouldWait(&waitingForBuffers))
            pthread_cond_wait(stateCond, &stateLock);
        if (stopping)
            break;

        if (stateChanged) {
            struct lorie_shared_server_state* oldState = nullptr;
            if (state && pendingState != state)
                oldState = state;

            state = pendingState;
            pendingState = nullptr;
            stateChanged = false;
            waitingForBuffers = false;

            if (state)
                state->surfaceAvailable = win != defaultWin;
            else if (win != defaultWin) {
                glClearColor(0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT);
                eglSwapBuffers(egl_display, sfc);
            }

            if (oldState)
                munmap(oldState, sizeof(*oldState));
        }

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

        pthread_cond_signal(&stateChangeFinishCond);
        pthread_mutex_unlock(&stateLock);

        // Prefer a full redraw over the standalone apply below so a pending GPU copy shares one
        // lock+fence with the root/cursor draw, instead of two GPU round trips per frame.
        bool gpuCopyPending = state && state->gpuCopyQueue.readIndex != state->gpuCopyQueue.writeIndex;
        if (state && state->surfaceAvailable && !state->waitForNextFrame &&
            (state->drawRequested || state->cursor.moved || state->cursor.updated || gpuCopyPending))
            redrawLocked(&waitingForBuffers);
        else if (gpuCopyPending)
            applyPendingGpuCopies();

        pthread_spin_lock(&bufferLock);
        // Remove all buffers which were attached to GL.
        while((buf = LorieBufferList_first(&removedBuffers)))
            LorieBuffer_release(buf);
        pthread_spin_unlock(&bufferLock);
        pthread_mutex_lock(&stateLock);
    }
    pthread_mutex_unlock(&stateLock);

    // Runs on the renderer thread: safe to touch GL/EGL here, they are thread-affine.
    removeAllBuffers();
    pthread_spin_lock(&bufferLock);
    while ((buf = LorieBufferList_first(&removedBuffers)))
        LorieBuffer_release(buf);
    pthread_spin_unlock(&bufferLock);

    if (state) {
        munmap(state, sizeof(*state));
        state = nullptr;
    }

    glDeleteProgram(g_texture_program);
    glDeleteProgram(g_texture_program_bgra);
    glDeleteTextures(1, &cursor.id);
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (sfc != defaultSfc) // defensive; setWindow(nullptr) already reverts this before onDetachedFromWindow runs
        eglDestroySurface(egl_display, sfc);
    eglDestroySurface(egl_display, defaultSfc);
    eglDestroyContext(egl_display, ctx);
    // Intentionally not calling eglTerminate(egl_display): the EGLDisplay is a process-wide
    // driver connection, not a per-instance resource.
    ANativeWindow_release(defaultWin);
    munmap(stateCond, sizeof(pthread_cond_t));
    close(stateCondFd);
    jvm->DetachCurrentThread();
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

void Renderer::drawCursor(float displayWidth, float displayHeight, float sourceLeft, float sourceTop) {
    float x, y, w, h;

    if (!state->cursor.width || !state->cursor.height || !state->cursor.visible)
        return;

    x = 2.f * ((float) state->cursor.x - sourceLeft - (float) state->cursor.xhot) / displayWidth - 1.f;
    y = 2.f * ((float) state->cursor.y - sourceTop - (float) state->cursor.yhot) / displayHeight - 1.f;
    w = 2.f * (float) state->cursor.width / displayWidth;
    h = 2.f * (float) state->cursor.height / displayHeight;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawRegion(cursor.id, x, y, x + w, y + h, 0.f, 0.f, 1.f, 1.f, false);
    glDisable(GL_BLEND);
}
