#include "dix-config.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>

#include "glamor_priv.h"

#define glog(...) __android_log_print(ANDROID_LOG_ERROR, "lorie-glamor", __VA_ARGS__)

static EGLDisplay lorieEglDisplay = EGL_NO_DISPLAY;
static EGLContext lorieEglContext = EGL_NO_CONTEXT;
static EGLSurface lorieEglSurface = EGL_NO_SURFACE;

struct lorieGlFunctions lorieGl;

int epoxy_gl_version(void) {
    int major = 0, minor = 0;
    const char *v = (const char *) glGetString(GL_VERSION);
    if (v && sscanf(v, "OpenGL ES %d.%d", &major, &minor) == 2)
        return major * 10 + minor;
    return 0;
}

int epoxy_has_gl_extension(const char *extension) {
    static const struct { const char *name; int coreSince; } core[] = {
        {"GL_OES_vertex_array_object", 30}, {"GL_OES_texture_border_clamp", 32}, {"GL_EXT_map_buffer_range", 30},
        {"GL_EXT_texture_rg", 30}, {"GL_EXT_unpack_subimage", 30}, {"GL_NV_pack_subimage", 30},
    };
    const char *all = (const char *) glGetString(GL_EXTENSIONS);
    size_t len = strlen(extension);
    for (size_t i = 0; i < sizeof(core) / sizeof(core[0]); i++)
        if (!strcmp(extension, core[i].name) && epoxy_gl_version() >= core[i].coreSince)
            return 1;
    for (const char *p = all; p && (p = strstr(p, extension)); p += len)
        if ((p == all || p[-1] == ' ') && (p[len] == ' ' || !p[len]))
            return 1;
    return 0;
}

int epoxy_is_desktop_gl(void) {
    return 0;
}

int epoxy_has_egl_extension(EGLDisplay dpy, const char *extension) {
    const char *all = eglQueryString(dpy, EGL_EXTENSIONS);
    size_t len = strlen(extension);
    for (const char *p = all; p && (p = strstr(p, extension)); p += len)
        if ((p == all || p[-1] == ' ') && (p[len] == ' ' || !p[len]))
            return 1;
    return 0;
}

static void *lorieGlLookup(const char *name) {
    static void *gles3;
    static int opened;
    void *fn = (void *) eglGetProcAddress(name);
    if (!fn && !opened) {
        opened = 1;
        gles3 = dlopen("libGLESv3.so", RTLD_NOW);
    }
    if (!fn && gles3)
        fn = dlsym(gles3, name);
    if (!fn)
        fn = dlsym(RTLD_DEFAULT, name);
    return fn;
}

// Some drivers do not list EGL_ANDROID_native_fence_sync to applications while still supporting it, so it is tried instead.
static Bool lorieNativeFenceWorks(EGLDisplay display) {
    EGLSyncKHR sync = lorieGl.eglCreateSync(display, EGL_SYNC_NATIVE_FENCE_ANDROID, NULL);
    int fd;

    if (sync == EGL_NO_SYNC_KHR)
        return FALSE;
    glFlush();
    fd = lorieGl.eglDupNativeFenceFd(display, sync);
    lorieGl.eglDestroySync(display, sync);
    if (fd == EGL_NO_NATIVE_FENCE_FD_ANDROID)
        return FALSE;
    close(fd);
    return TRUE;
}

static Bool lorieGlLoad(EGLDisplay display) {
    lorieGl.bindVertexArray = lorieGlLookup("glBindVertexArray");
    lorieGl.deleteVertexArrays = lorieGlLookup("glDeleteVertexArrays");
    lorieGl.genVertexArrays = lorieGlLookup("glGenVertexArrays");
    lorieGl.drawArraysInstanced = lorieGlLookup("glDrawArraysInstanced");
    lorieGl.mapBufferRange = lorieGlLookup("glMapBufferRange");
    lorieGl.unmapBuffer = lorieGlLookup("glUnmapBuffer");
    lorieGl.uniform4ui = lorieGlLookup("glUniform4ui");
    lorieGl.vertexAttribDivisor = lorieGlLookup("glVertexAttribDivisor");
    lorieGl.debugMessageCallback = lorieGlLookup("glDebugMessageCallback");
    lorieGl.debugMessageControl = lorieGlLookup("glDebugMessageControl");
    lorieGl.objectLabel = lorieGlLookup("glObjectLabel");
    if (!lorieGl.bindVertexArray || !lorieGl.deleteVertexArrays || !lorieGl.genVertexArrays ||
        !lorieGl.drawArraysInstanced || !lorieGl.mapBufferRange || !lorieGl.unmapBuffer ||
        !lorieGl.uniform4ui || !lorieGl.vertexAttribDivisor) {
        glog("GLES 3 entry points are missing");
        return FALSE;
    }

    lorieGl.eglCreateSync = (PFNEGLCREATESYNCKHRPROC) lorieGlLookup("eglCreateSyncKHR");
    lorieGl.eglDestroySync = (PFNEGLDESTROYSYNCKHRPROC) lorieGlLookup("eglDestroySyncKHR");
    lorieGl.eglDupNativeFenceFd = (PFNEGLDUPNATIVEFENCEFDANDROIDPROC) lorieGlLookup("eglDupNativeFenceFDANDROID");
    if (!lorieGl.eglCreateSync || !lorieGl.eglDestroySync || !lorieGl.eglDupNativeFenceFd) {
        glog("EGL fence sync entry points are missing");
        return FALSE;
    }

    if (!lorieNativeFenceWorks(display)) {
        glog("exporting native fences does not work");
        return FALSE;
    }

    return TRUE;
}

static void lorieGlamorMakeCurrent(struct glamor_context *ctx) {
    eglMakeCurrent(ctx->display, ctx->drawable, ctx->drawable, ctx->ctx);
}

Bool lorieGlamorEglInit(void) {
    static const EGLint configAttribs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                                           EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE};
    static const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    static const EGLint pbufferAttribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    EGLConfig config;
    EGLint major, minor, count;

    if (lorieEglContext != EGL_NO_CONTEXT)
        return eglMakeCurrent(lorieEglDisplay, lorieEglSurface, lorieEglSurface, lorieEglContext);

    lorieEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (lorieEglDisplay == EGL_NO_DISPLAY || !eglInitialize(lorieEglDisplay, &major, &minor))
        return FALSE;
    eglBindAPI(EGL_OPENGL_ES_API);
    if (!eglChooseConfig(lorieEglDisplay, configAttribs, &config, 1, &count) || !count)
        return FALSE;
    lorieEglContext = eglCreateContext(lorieEglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    lorieEglSurface = eglCreatePbufferSurface(lorieEglDisplay, config, pbufferAttribs);
    if (lorieEglContext == EGL_NO_CONTEXT || lorieEglSurface == EGL_NO_SURFACE)
        return FALSE;
    return eglMakeCurrent(lorieEglDisplay, lorieEglSurface, lorieEglSurface, lorieEglContext);
}

void lorieGlamorFinish(void) {
    glFinish();
}

Bool lorieGlamorInit(ScreenPtr screen) {
    if (!lorieGlamorEglInit()) {
        glog("EGL init failed: 0x%x", eglGetError());
        return FALSE;
    }
    if (!lorieGlLoad(lorieEglDisplay))
        return FALSE;
    if (!glamor_init(screen, GLAMOR_USE_EGL_SCREEN | GLAMOR_NO_DRI3)) {
        glog("glamor_init failed");
        return FALSE;
    }
    return TRUE;
}

void glTextureBarrierNV(void) {}
void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void *data) {}
void glLogicOp(GLenum op) {}
void *glMapBuffer(GLenum target, GLenum access) { return NULL; }
void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const char *name) {}
void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags) {}

void glamor_egl_screen_init(ScreenPtr screen, struct glamor_context *glamor_ctx) {
    glamor_ctx->display = lorieEglDisplay;
    glamor_ctx->ctx = lorieEglContext;
    glamor_ctx->drawable = lorieEglSurface;
    glamor_ctx->make_current = lorieGlamorMakeCurrent;
}

Bool glamor_glx_screen_init(struct glamor_context *glamor_ctx) {
    return FALSE;
}

int glamor_egl_fd_name_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, CARD16 *stride, CARD32 *size) {
    return -1;
}

int glamor_egl_fds_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, int *fds, uint32_t *offsets, uint32_t *strides,
                               uint64_t *modifier) {
    return 0;
}

int glamor_egl_fd_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, CARD16 *stride, CARD32 *size) {
    return -1;
}
