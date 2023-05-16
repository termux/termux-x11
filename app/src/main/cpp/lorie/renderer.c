#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "DanglingPointer"
#pragma ide diagnostic ignored "ConstantConditionsOC"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GL/gl.h>
#include <android/native_window.h>
#include <android/log.h>
#include <dlfcn.h>
#include "renderer.h"
#include "os.h"

// We can not link both mesa's GL and Android's GLES without interfering.
// That is the only way to do this without creating linker namespaces.

static EGLDisplay (*$eglGetDisplay)(EGLNativeDisplayType nativeDisplay) = NULL;
static EGLint (*$eglGetError)(void) = NULL;
static EGLBoolean (*$eglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor) = NULL;
static EGLBoolean (*$eglChooseConfig)(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) = NULL;
static EGLBoolean (*$eglBindAPI)(EGLenum api) = NULL;
static EGLContext (*$eglCreateContext)(EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list) = NULL;
static EGLBoolean (*$eglMakeCurrent)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) = NULL;
static EGLBoolean (*$eglDestroySurface)(EGLDisplay dpy, EGLSurface surface) = NULL;
static EGLClientBuffer (*$eglGetNativeClientBufferANDROID)(const struct AHardwareBuffer *buffer);
static EGLImageKHR (*$eglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
static EGLBoolean (*$eglDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image);
static EGLSurface (*$eglCreateWindowSurface)(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType window, const EGLint *attrib_list) = NULL;
static EGLBoolean (*$eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = NULL;

static GLenum (*$glGetError)(void);
static GLuint (*$glCreateShader)(GLenum type) = NULL;
static void (*$glShaderSource)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) = NULL;
static void (*$glCompileShader)(GLuint shader) = NULL;
static void (*$glGetShaderiv)(GLuint shader, GLenum pname, GLint *params) = NULL;
static void (*$glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) = NULL;
static void (*$glDeleteShader)(GLuint shader) = NULL;
static GLuint (*$glCreateProgram)(void) = NULL;
static void (*$glAttachShader)(GLuint program, GLuint shader) = NULL;
static void (*$glLinkProgram)(GLuint program) = NULL;
static void (*$glGetProgramiv)(GLuint program, GLenum pname, GLint *params) = NULL;
static void (*$glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) = NULL;
static void (*$glDeleteProgram)(GLuint program) = NULL;
static void (*$glActiveTexture)(GLenum texture) = NULL;
static void (*$glUseProgram)(GLuint program) = NULL;
static void (*$glBindTexture)(GLenum target, GLuint texture) = NULL;
static void (*$glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) = NULL;
static void (*$glEnableVertexAttribArray)(GLuint index) = NULL;
static void (*$glDrawArrays)(GLenum mode, GLint first, GLsizei count) = NULL;
static void (*$glEnable)(GLenum cap) = NULL;
static void (*$glBlendFunc)(GLenum sfactor, GLenum dfactor) = NULL;
static void (*$glDisable)(GLenum cap) = NULL;
static GLint (*$glGetAttribLocation)(GLuint program, const GLchar *name) = NULL;
static void (*$glGenTextures)(GLsizei n, GLuint *textures) = NULL;
static void (*$glViewport)(GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
static void (*$glClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = NULL;
static void (*$glClear)(GLbitfield mask) = NULL;
static void (*$glTexParameteri)(GLenum target, GLenum pname, GLint param) = NULL;
static void (*$glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) = NULL;
static void (*$glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) = NULL;
static void (*$glEGLImageTargetTexture2DOES)(GLenum target, GLeglImageOES image) = NULL;

#define SYMBOL(lib, name) $ ## name = dlsym(lib, #name)
static void init(void) {
    void *libEGL, *libGLESv2;
    if ($eglGetDisplay)
        return;
    libEGL = dlopen("libEGL.so", RTLD_NOW);
    SYMBOL(libEGL, eglGetDisplay);
    SYMBOL(libEGL, eglGetError);
    SYMBOL(libEGL, eglInitialize);
    SYMBOL(libEGL, eglChooseConfig);
    SYMBOL(libEGL, eglBindAPI);
    SYMBOL(libEGL, eglCreateContext);
    SYMBOL(libEGL, eglMakeCurrent);
    SYMBOL(libEGL, eglDestroySurface);
    SYMBOL(libEGL, eglGetNativeClientBufferANDROID);
    SYMBOL(libEGL, eglCreateImageKHR);
    SYMBOL(libEGL, eglDestroyImageKHR);
    SYMBOL(libEGL, eglCreateWindowSurface);
    SYMBOL(libEGL, eglSwapBuffers);

    libGLESv2 = dlopen("libGLESv2.so", RTLD_NOW);
    SYMBOL(libGLESv2, glGetError);
    SYMBOL(libGLESv2, glCreateShader);
    SYMBOL(libGLESv2, glShaderSource);
    SYMBOL(libGLESv2, glCompileShader);
    SYMBOL(libGLESv2, glGetShaderiv);
    SYMBOL(libGLESv2, glGetShaderInfoLog);
    SYMBOL(libGLESv2, glDeleteShader);
    SYMBOL(libGLESv2, glCreateProgram);
    SYMBOL(libGLESv2, glAttachShader);
    SYMBOL(libGLESv2, glLinkProgram);
    SYMBOL(libGLESv2, glGetProgramiv);
    SYMBOL(libGLESv2, glGetProgramInfoLog);
    SYMBOL(libGLESv2, glDeleteProgram);
    SYMBOL(libGLESv2, glActiveTexture);
    SYMBOL(libGLESv2, glUseProgram);
    SYMBOL(libGLESv2, glBindTexture);
    SYMBOL(libGLESv2, glVertexAttribPointer);
    SYMBOL(libGLESv2, glEnableVertexAttribArray);
    SYMBOL(libGLESv2, glDrawArrays);
    SYMBOL(libGLESv2, glEnable);
    SYMBOL(libGLESv2, glBlendFunc);
    SYMBOL(libGLESv2, glDisable);
    SYMBOL(libGLESv2, glGetAttribLocation);
    SYMBOL(libGLESv2, glActiveTexture);
    SYMBOL(libGLESv2, glGenTextures);
    SYMBOL(libGLESv2, glViewport);
    SYMBOL(libGLESv2, glClearColor);
    SYMBOL(libGLESv2, glClear);
    SYMBOL(libGLESv2, glTexParameteri);
    SYMBOL(libGLESv2, glTexImage2D);
    SYMBOL(libGLESv2, glTexSubImage2D);
    SYMBOL(libGLESv2, glEGLImageTargetTexture2DOES);
}

// We need some stub to avoid segfaulting
static void voidMessage(maybe_unused MessageType type, maybe_unused int verb, maybe_unused const char *format, ...) {}

static renderer_message_func_type logMessage = voidMessage;
void renderer_message_func(renderer_message_func_type function) {
    logMessage = function;
}

//#define log(...) logMessage(X_ERROR, -1, __VA_ARGS__)
#define log(...) __android_log_print(ANDROID_LOG_DEBUG, "gles-renderer", __VA_ARGS__)

static GLuint create_program(const char* p_vertex_source, const char* p_fragment_source);

static void eglCheckError(int line) {
    char* desc;
    switch($eglGetError()) {
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
        default: desc = (char*) "Unknown error";
    }

    if (desc)
        log("Exagear-renderer: egl error on line %d: %s\n", line, desc);
}

static void checkGlError(int line) {
    GLenum error;
    char *desc = NULL;
    for (error = $glGetError(); error; error = $glGetError()) {
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
        log("Exagear-renderer: GLES %d ERROR: %s.\n", line, desc);
        return;
    }
    //LOGE("Last op on line %d was successfull", line);
}

#define checkGlError() checkGlError(__LINE__)


static const char vertex_shader[] =
    "attribute vec4 position;\n"
    "attribute vec2 texCoords;"
    "varying vec2 outTexCoords;\n"
    "void main(void) {\n"
    "   outTexCoords = texCoords;\n"
    "   gl_Position = position;\n"
    "}\n";
static const char fragment_shader[] =
    "precision mediump float;\n"
    "varying vec2 outTexCoords;\n"
    "uniform sampler2D texture;\n"
    "void main(void) {\n"
    "   gl_FragColor = texture2D(texture, outTexCoords).bgra;\n"
    "}\n";

static EGLDisplay egl_display = EGL_NO_DISPLAY;
static EGLContext ctx = EGL_NO_CONTEXT;
static EGLSurface sfc = EGL_NO_SURFACE;
static EGLConfig cfg = 0;
static EGLNativeWindowType win = 0;
static EGLImageKHR image = NULL;

static struct {
    GLuint id;
    float width, height;
} display;
static struct {
    GLuint id;
    float x, y, width, height, xhot, yhot;
} cursor;

GLuint g_texture_program = 0, gv_pos = 0, gv_coords = 0;

int renderer_init(void) {
    EGLint major, minor;
    EGLint numConfigs;
    const EGLint configAttribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_NONE
    };
    const EGLint ctxattribs[] = {
            EGL_CONTEXT_CLIENT_VERSION,2, EGL_NONE
    };

    if (ctx)
        return 1;
    init();

    egl_display = $eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglCheckError(__LINE__);
    if (egl_display == EGL_NO_DISPLAY) {
        log("Exagear-renderer: Got no EGL display.\n");
        return 0;
    }

    if ($eglInitialize(egl_display, &major, &minor) != EGL_TRUE) {
        log("Exagear-renderer: Unable to initialize EGL\n");
        return 0;
    }
    log("Exagear-renderer: Initialized EGL version %d.%d\n", major, minor);
    eglCheckError(__LINE__);

    if ($eglChooseConfig(egl_display, configAttribs, &cfg, 1, &numConfigs) != EGL_TRUE) {
        log("Exagear-renderer: eglChooseConfig failed.\n");
        eglCheckError(__LINE__);
        return 0;
    }

    $eglBindAPI(EGL_OPENGL_ES_API);
    eglCheckError(__LINE__);

    ctx = $eglCreateContext(egl_display, cfg, NULL, ctxattribs);
    eglCheckError(__LINE__);
    if (ctx == EGL_NO_CONTEXT) {
        log("Exagear-renderer: eglCreateContext failed.\n");
        eglCheckError(__LINE__);
        return 0;
    }

    if ($eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE) {
        log("Exagear-renderer: eglMakeCurrent failed.\n");
        eglCheckError(__LINE__);
        return 0;
    }
    eglCheckError(__LINE__);

    g_texture_program = create_program(vertex_shader, fragment_shader);
    if (!g_texture_program) {
        log("Exagear-renderer: GLESv2: Unable to create shader program.\n");
        eglCheckError(__LINE__);
        return 1;
    }

    gv_pos = (GLuint) $glGetAttribLocation(g_texture_program, "position"); checkGlError();
    gv_coords = (GLuint) $glGetAttribLocation(g_texture_program, "texCoords"); checkGlError();

    $glActiveTexture(GL_TEXTURE0); checkGlError();
    $glGenTextures(1, &display.id); checkGlError();
    $glGenTextures(1, &cursor.id); checkGlError();

    return 1;
}

void renderer_set_buffer(AHardwareBuffer* buffer) {
    const EGLint imageAttributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
    EGLClientBuffer clientBuffer;
    AHardwareBuffer_Desc desc;
    __android_log_print(ANDROID_LOG_DEBUG, "XegwTest2", "renderer_set_buffer0");
    if (image)
        $eglDestroyImageKHR(egl_display, image);

    AHardwareBuffer_describe(buffer, &desc);

    display.width = (float) desc.width;
    display.height = (float) desc.height;

    clientBuffer = $eglGetNativeClientBufferANDROID(buffer); eglCheckError(__LINE__);
    image = $eglCreateImageKHR(egl_display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, imageAttributes); eglCheckError(__LINE__);

    $glBindTexture(GL_TEXTURE_2D, display.id); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); checkGlError();
    $glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image); checkGlError();
    renderer_redraw();

    __android_log_print(ANDROID_LOG_DEBUG, "XegwTest2", "renderer_set_buffer %d %d", desc.width, desc.height);
}

void renderer_set_window(EGLNativeWindowType window) {
    __android_log_print(ANDROID_LOG_DEBUG, "XegwTest2", "renderer_set_window %p %d %d", window, win ? ANativeWindow_getWidth(win) : 0, win ? ANativeWindow_getHeight(win) : 0);
    if (win == window)
        return;

    if (sfc != EGL_NO_SURFACE) {
        if ($eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE) {
            log("Exagear-renderer: eglMakeCurrent (EGL_NO_SURFACE) failed.\n");
            eglCheckError(__LINE__);
            return;
        }
        if ($eglDestroySurface(egl_display, sfc) != EGL_TRUE) {
            log("Exagear-renderer: eglDestoySurface failed.\n");
            eglCheckError(__LINE__);
            return;
        }
    }

    if (win)
        ANativeWindow_release(win);
    win = window;

    sfc = $eglCreateWindowSurface(egl_display, cfg, win, NULL);
    if (sfc == EGL_NO_SURFACE) {
        log("Exagear-renderer: eglCreateWindowSurface failed.\n");
        eglCheckError(__LINE__);
        return;
    }

    if ($eglMakeCurrent(egl_display, sfc, sfc, ctx) != EGL_TRUE) {
        log("Exagear-renderer: eglMakeCurrent failed.\n");
        eglCheckError(__LINE__);
        return;
    }

    if (win && ctx && ANativeWindow_getWidth(win) && ANativeWindow_getHeight(win))
        $glViewport(0, 0, ANativeWindow_getWidth(win), ANativeWindow_getHeight(win)); checkGlError();

    log("Exagear-renderer: new surface applied: %p\n", sfc);

    $glClearColor(1.f, 0.f, 0.f, 0.0f); checkGlError();
    $glClear(GL_COLOR_BUFFER_BIT); checkGlError();
    renderer_redraw();
    $eglSwapBuffers(egl_display, sfc);
}

maybe_unused void renderer_upload(int w, int h, void* data) {
    display.width = (float) w;
    display.height = (float) h;
    $glBindTexture(GL_TEXTURE_2D, display.id); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); checkGlError();

    $glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); checkGlError();
}

maybe_unused void renderer_update_rects(int width, maybe_unused int height, pixman_box16_t *rects, int amount, void* data) {
    int i, w, j;
    uint32_t* d;
    display.width = (float) width;
    display.height = (float) height;
    $glBindTexture(GL_TEXTURE_2D, display.id); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); checkGlError();

    for (i = 0; i < amount; i++)
        for (j = rects[i].y1; j < rects[i].y2; j++) {
            d = &((uint32_t*)data)[width * j + rects[i].x1];
            w = rects[i].x2 - rects[i].x1;
            $glTexSubImage2D(GL_TEXTURE_2D, 0, rects[i].x1, j, w, 1, GL_RGBA, GL_UNSIGNED_BYTE, d); checkGlError();
        }
}

void renderer_update_cursor(int w, int h, int xhot, int yhot, void* data) {
    log("Exagear-renderer: updating cursor\n");
    cursor.width = (float) w;
    cursor.height = (float) h;
    cursor.xhot = (float) xhot;
    cursor.yhot = (float) yhot;

    $glBindTexture(GL_TEXTURE_2D, cursor.id); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); checkGlError();
    $glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); checkGlError();

    $glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); checkGlError();
}

void renderer_set_cursor_coordinates(int x, int y) {
    cursor.x = (float) x;
    cursor.y = (float) y;
}

static void draw(GLuint id, float x0, float y0, float x1, float y1);
static void draw_cursor(void);

float ia = 0;

void renderer_redraw(void) {
    draw(display.id,  -1.f, -1.f, 1.f, 1.f);
    draw_cursor();
    $eglSwapBuffers(egl_display, sfc); checkGlError();
}

static GLuint load_shader(GLenum shaderType, const char* pSource) {
    GLint compiled = 0;
    GLuint shader = $glCreateShader(shaderType); checkGlError();
    if (shader) {
        $glShaderSource(shader, 1, &pSource, NULL); checkGlError();
        $glCompileShader(shader); checkGlError();
        $glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled); checkGlError();
        if (!compiled) {
            GLint infoLen = 0;
            $glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen); checkGlError();
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    $glGetShaderInfoLog(shader, infoLen, NULL, buf); checkGlError();
                    log("Exagear-renderer: Could not compile shader %d:\n%s\n", shaderType, buf);
                    free(buf);
                }
                $glDeleteShader(shader); checkGlError();
                shader = 0;
            }
        }
    }
    return shader;
}

static GLuint create_program(const char* p_vertex_source, const char* p_fragment_source) {
    GLuint program, vertexShader, pixelShader;
    GLint linkStatus = GL_FALSE;
    vertexShader = load_shader(GL_VERTEX_SHADER, p_vertex_source);
    pixelShader = load_shader(GL_FRAGMENT_SHADER, p_fragment_source);
    if (!pixelShader || !vertexShader) {
        return 0;
    }

    program = $glCreateProgram(); checkGlError();
    if (program) {
        $glAttachShader(program, vertexShader); checkGlError();
        $glAttachShader(program, pixelShader); checkGlError();
        $glLinkProgram(program); checkGlError();
        $glGetProgramiv(program, GL_LINK_STATUS, &linkStatus); checkGlError();
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            $glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength); checkGlError();
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    $glGetProgramInfoLog(program, bufLength, NULL, buf); checkGlError();
                    log("Exagear-renderer: Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            $glDeleteProgram(program); checkGlError();
            program = 0;
        }
    }
    return program;
}

static void draw(GLuint id, float x0, float y0, float x1, float y1) {
    float coords[20] = {
        x0, -y0, 0.f, 0.f, 0.f,
        x1, -y0, 0.f, 1.f, 0.f,
        x0, -y1, 0.f, 0.f, 1.f,
        x1, -y1, 0.f, 1.f, 1.f,
    };

    $glActiveTexture(GL_TEXTURE0); checkGlError();
    $glUseProgram(g_texture_program); checkGlError();
    $glBindTexture(GL_TEXTURE_2D, id); checkGlError();

    $glVertexAttribPointer(gv_pos, 3, GL_FLOAT, GL_FALSE, 20, coords); checkGlError();
    $glVertexAttribPointer(gv_coords, 2, GL_FLOAT, GL_FALSE, 20, &coords[3]); checkGlError();
    $glEnableVertexAttribArray(gv_pos); checkGlError();
    $glEnableVertexAttribArray(gv_coords); checkGlError();
    $glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); checkGlError();
}

maybe_unused static void draw_cursor(void) {
    float x, y, w, h;
    x = 2 * (cursor.x - cursor.xhot) / display.width - 1;
    y = 2 * (cursor.y - cursor.yhot) / display.height - 1;
    w = 2 * cursor.width / display.width;
    h = 2 * cursor.height / display.height;
    $glEnable(GL_BLEND); checkGlError();
    $glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); checkGlError();
    draw(cursor.id, x, y, x + w, y + h);
    $glDisable(GL_BLEND); checkGlError();
}

