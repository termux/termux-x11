#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "DanglingPointer"
#pragma ide diagnostic ignored "ConstantConditionsOC"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "UnreachableCode"
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#pragma ide diagnostic ignored "misc-no-recursion"
#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include "renderer.h"
#include "os.h"

#define log(...) __android_log_print(ANDROID_LOG_DEBUG, "gles-renderer", __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, "gles-renderer", __VA_ARGS__)

static GLuint create_program(const char* p_vertex_source, const char* p_fragment_source);

static int printEglError(char* msg, int ret, int line) {
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

    return ret;
}

static inline __always_inline void vprintEglError(char* msg, int line) {
    printEglError(msg, 0, line);
}

static const char* eglErrorLabel(int code) {
    switch(code) {
        case EGL_SUCCESS: return NULL; // "No error"
#define E(code) case code: return #code; break
        E(EGL_NOT_INITIALIZED);
        E(EGL_BAD_ACCESS);
        E(EGL_BAD_ALLOC);
        E(EGL_BAD_ATTRIBUTE);
        E(EGL_BAD_CONTEXT);
        E(EGL_BAD_CONFIG);
        E(EGL_BAD_CURRENT_SURFACE);
        E(EGL_BAD_DISPLAY);
        E(EGL_BAD_SURFACE);
        E(EGL_BAD_MATCH);
        E(EGL_BAD_PARAMETER);
        E(EGL_BAD_NATIVE_PIXMAP);
        E(EGL_BAD_NATIVE_WINDOW);
        E(EGL_CONTEXT_LOST);
#undef E
        default: return "EGL_UNKNOWN_ERROR";
    }

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


static const char vertex_shader[] =
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

static const char fragment_shader[] = FRAGMENT_SHADER();
static const char fragment_shader_bgra[] = FRAGMENT_SHADER(".bgra");

static EGLDisplay egl_display = EGL_NO_DISPLAY;
static EGLContext ctx = EGL_NO_CONTEXT;
static EGLSurface sfc = EGL_NO_SURFACE;
static EGLConfig cfg = 0;
static EGLNativeWindowType win = 0;
static jobject surface = NULL;
static AHardwareBuffer *buffer = NULL;
static EGLImageKHR image = NULL;
static int renderedFrames = 0;

static jmethodID Surface_release = NULL;
static jmethodID Surface_destroy = NULL;

static struct {
    GLuint id;
    float width, height;
} display;
static struct {
    GLuint id;
    float x, y, width, height, xhot, yhot;
} cursor;

GLuint g_texture_program = 0, gv_pos = 0, gv_coords = 0;
GLuint g_texture_program_bgra = 0, gv_pos_bgra = 0, gv_coords_bgra = 0;

static inline __always_inline void bindLinearTexture(GLuint id) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

int renderer_init(JNIEnv* env, int* legacy_drawing, uint8_t* flip) {
    EGLint major, minor;
    EGLint numConfigs;
    EGLint configAttribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 0,
            EGL_NONE
    };
    const EGLint ctxattribs[] = {
            EGL_CONTEXT_CLIENT_VERSION,2, EGL_NONE
    };
    EGLint * const alphaAttrib = &configAttribs[11];

    if (ctx)
        return 1;

    jclass Surface = (*env)->FindClass(env, "android/view/Surface");
    Surface_release = (*env)->GetMethodID(env, Surface, "release", "()V");
    Surface_destroy = (*env)->GetMethodID(env, Surface, "destroy", "()V");
    if (!Surface_release) {
        loge("Failed to find required Surface.release method. Aborting.\n");
        abort();
    }
    if (!Surface_destroy) {
        loge("Failed to find required Surface.destroy method. Aborting.\n");
        abort();
    }

    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY)
        return printEglError("Got no EGL display", 0, __LINE__);

    if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE)
        return printEglError("Unable to initialize EGL", 0, __LINE__);

    log("Xlorie: Initialized EGL version %d.%d\n", major, minor);
    eglBindAPI(EGL_OPENGL_ES_API);

    if (eglChooseConfig(egl_display, configAttribs, &cfg, 1, &numConfigs) != EGL_TRUE &&
            (*alphaAttrib = 8) &&
            eglChooseConfig(egl_display, configAttribs, &cfg, 1, &numConfigs) != EGL_TRUE)
        return printEglError("eglChooseConfig failed", 0, __LINE__);

    ctx = eglCreateContext(egl_display, cfg, NULL, ctxattribs);
    if (ctx == EGL_NO_CONTEXT)
        return printEglError("eglCreateContext failed", 0, __LINE__);

    if (eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
        return printEglError("eglMakeCurrent failed", 0, __LINE__);

    {
        // Some devices do not support sampling from HAL_PIXEL_FORMAT_BGRA_8888, here we are checking it.
        const EGLint imageAttributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
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

        status = AHardwareBuffer_allocate(&d0, &new);
        if (status != 0 || new == NULL) {
            loge("Failed to allocate native buffer (%p, error %d)", new, status);
            loge("Forcing legacy drawing");
            *legacy_drawing = 1;
            return 1;
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
            return 1;
        }

        clientBuffer = eglGetNativeClientBufferANDROID(new);
        if (!clientBuffer) {
            *legacy_drawing = 1;
            AHardwareBuffer_release(new);
            return printEglError("Failed to obtain EGLClientBuffer from AHardwareBuffer, forcing legacy drawing", 1, __LINE__);
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
                return printEglError("check eglChooseConfig failed", 0, __LINE__);

            EGLContext testctx = eglCreateContext(egl_display, checkcfg, NULL, ctxattribs);
            if (testctx == EGL_NO_CONTEXT)
                return printEglError("check eglCreateContext failed", 0, __LINE__);

            const EGLint pbufferAttributes[] = {
                    EGL_WIDTH, 64,
                    EGL_HEIGHT, 64,
                    EGL_NONE,
            };
            EGLSurface checksfc = eglCreatePbufferSurface(egl_display, checkcfg, pbufferAttributes);

            if (eglMakeCurrent(egl_display, checksfc, checksfc, testctx) != EGL_TRUE)
                return printEglError("check eglMakeCurrent failed", 0, __LINE__);

            glActiveTexture(GL_TEXTURE0); checkGlError();
            glGenTextures(1, &texture); checkGlError();
            bindLinearTexture(texture);
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

    return 1;
}

static void renderer_unset_buffer(void) {
    if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
        loge("There is no current context, `renderer_set_buffer` call is cancelled");
        return;
    }

    log("renderer_set_buffer0");
    if (image)
        eglDestroyImageKHR(egl_display, image);
    if (buffer)
        AHardwareBuffer_release(buffer);

    buffer = NULL;
}

void renderer_set_buffer(JNIEnv* env, AHardwareBuffer* buf) {
    const EGLint imageAttributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
    AHardwareBuffer_Desc desc = {0};
    uint8_t flip = 0;

    if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
        loge("There is no current context, `renderer_set_buffer` call is cancelled");
        return;
    }

    renderer_unset_buffer();

    buffer = buf;

    bindLinearTexture(display.id);
    if (buffer) {
        AHardwareBuffer_acquire(buffer);
        AHardwareBuffer_describe(buffer, &desc);

        display.width = (float) desc.width;
        display.height = (float) desc.height;

        image = eglCreateImageKHR(egl_display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, eglGetNativeClientBufferANDROID(buffer), imageAttributes);
        if (image != NULL) {
            glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
            flip = desc.format != AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM;
        } else {
            display.width = 1;
            display.height = 1;
            uint32_t data = {0};
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data);
        }
    } else {
        display.width = 1;
        display.height = 1;
        uint32_t data = {0};
        loge("There is no AHardwareBuffer, nothing to be bound.");
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data);
    }

    renderer_redraw(env, flip);

    log("renderer_set_buffer %p %d %d", buffer, desc.width, desc.height);
}

static inline __always_inline void release_win_and_surface(JNIEnv *env, jobject* jsfc, ANativeWindow** anw, EGLSurface *esfc) {
    if (esfc && *esfc) {
        if (eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
            return vprintEglError("eglMakeCurrent failed (EGL_NO_SURFACE)", __LINE__);
        if (eglDestroySurface(egl_display, *esfc) != EGL_TRUE)
            return vprintEglError("eglDestoySurface failed", __LINE__);
        *esfc = EGL_NO_SURFACE;
    }

    if (anw && *anw) {
        ANativeWindow_release(*anw);
        *anw = NULL;
    }

    if (jsfc && *jsfc) {
        (*env)->CallVoidMethod(env, *jsfc, Surface_release);
        (*env)->CallVoidMethod(env, *jsfc, Surface_destroy);
        (*env)->DeleteGlobalRef(env, *jsfc);
        *jsfc = NULL;
    }
}

void renderer_set_window(JNIEnv* env, jobject new_surface, AHardwareBuffer* new_buffer) {
    ANativeWindow* window;
    if (new_surface && surface && new_surface != surface && (*env)->IsSameObject(env, new_surface, surface)) {
        (*env)->DeleteGlobalRef(env, new_surface);
        new_surface = NULL;
        return;
    }

    window = new_surface ? ANativeWindow_fromSurface(env, new_surface) : NULL;
    int width = window ? ANativeWindow_getWidth(window) : 0;
    int height = window ? ANativeWindow_getHeight(window) : 0;
    log("renderer_set_window %p %d %d", window, width, height);
    if (window && win == window)
        return;

    if (window)
        ANativeWindow_acquire(window);

    release_win_and_surface(env, &surface, &win, &sfc);

    if (window && (width <= 0 || height <= 0)) {
        log("Xlorie: We've got invalid surface. Probably it became invalid before we started working with it.\n");
        release_win_and_surface(env, &new_surface, &window, NULL);
    }

    win = window;
    surface = new_surface;

    if (!win)
        return;

    sfc = eglCreateWindowSurface(egl_display, cfg, win, NULL);
    if (sfc == EGL_NO_SURFACE)
        return vprintEglError("eglCreateWindowSurface failed", __LINE__);

    if (eglMakeCurrent(egl_display, sfc, sfc, ctx) != EGL_TRUE)
        return vprintEglError("eglMakeCurrent failed", __LINE__);

    if (!g_texture_program) {
        g_texture_program = create_program(vertex_shader, fragment_shader);
        if (!g_texture_program) {
            log("Xlorie: GLESv2: Unable to create shader program.\n");
            return;
        }

        g_texture_program_bgra = create_program(vertex_shader, fragment_shader_bgra);
        if (!g_texture_program_bgra) {
            log("Xlorie: GLESv2: Unable to create bgra shader program.\n");
            return;
        }

        gv_pos = (GLuint) glGetAttribLocation(g_texture_program, "position");
        gv_coords = (GLuint) glGetAttribLocation(g_texture_program, "texCoords");

        gv_pos_bgra = (GLuint) glGetAttribLocation(g_texture_program_bgra, "position");
        gv_coords_bgra = (GLuint) glGetAttribLocation(g_texture_program_bgra, "texCoords");

        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &display.id);
        glGenTextures(1, &cursor.id);
    }

    eglSwapInterval(egl_display, 0);

    if (win && ctx && ANativeWindow_getWidth(win) > 0 && ANativeWindow_getHeight(win) > 0)
        glViewport(0, 0, ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));

    log("Xlorie: new surface applied: %p\n", sfc);

    if (!new_buffer) {
        glClearColor(0.f, 0.f, 0.f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    } else renderer_set_buffer(env, new_buffer);
}

void renderer_update_root(int w, int h, void* data, uint8_t flip) {
    if (eglGetCurrentContext() == EGL_NO_CONTEXT || !w || !h)
        return;

    bindLinearTexture(display.id);
    if (display.width != (float) w || display.height != (float) h)
        glTexImage2D(GL_TEXTURE_2D, 0, flip ? GL_RGBA : GL_BGRA_EXT, w, h, 0, flip ? GL_RGBA : GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, flip ? GL_RGBA : GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);

    display.width = (float) w;
    display.height = (float) h;
}

void renderer_update_cursor(int w, int h, int xhot, int yhot, void* data) {
    log("Xlorie: updating cursor\n");
    cursor.width = (float) w;
    cursor.height = (float) h;
    cursor.xhot = (float) xhot;
    cursor.yhot = (float) yhot;

    if (eglGetCurrentContext() == EGL_NO_CONTEXT || !cursor.width || !cursor.height)
        return;

    bindLinearTexture(cursor.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void renderer_set_cursor_coordinates(int x, int y) {
    cursor.x = (float) x;
    cursor.y = (float) y;
}

static void draw(GLuint id, float x0, float y0, float x1, float y1, uint8_t flip);
static void draw_cursor(void);

int renderer_should_redraw(void) {
    return sfc != EGL_NO_SURFACE && eglGetCurrentContext() != EGL_NO_CONTEXT;
}

int renderer_redraw(JNIEnv* env, uint8_t flip) {
    int err = EGL_SUCCESS;

    if (!sfc || eglGetCurrentContext() == EGL_NO_CONTEXT)
        return FALSE;

    draw(display.id,  -1.f, -1.f, 1.f, 1.f, flip);
    draw_cursor();
    if (eglSwapBuffers(egl_display, sfc) != EGL_TRUE) {
        printEglError("Failed to swap buffers", 0, __LINE__);
        err = eglGetError();
        if (err == EGL_BAD_NATIVE_WINDOW || err == EGL_BAD_SURFACE) {
            log("We've got %s so window is to be destroyed. "
                "Native window disconnected/abandoned, probably activity is destroyed or in background",
                eglErrorLabel(err));
            renderer_set_window(env, NULL, NULL);
            return FALSE;
        }
    }

    renderedFrames++;
    return TRUE;
}

void renderer_print_fps(float millis) {
    if (renderedFrames)
        log("%d frames in %.1f seconds = %.1f FPS",
                                renderedFrames, millis / 1000, (float) renderedFrames *  1000 / millis);
    renderedFrames = 0;
}

static GLuint load_shader(GLenum shaderType, const char* pSource) {
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

static GLuint create_program(const char* p_vertex_source, const char* p_fragment_source) {
    GLuint program, vertexShader, pixelShader;
    GLint linkStatus = GL_FALSE, bufLength = 0;
    vertexShader = load_shader(GL_VERTEX_SHADER, p_vertex_source);
    pixelShader = load_shader(GL_FRAGMENT_SHADER, p_fragment_source);
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

static void draw(GLuint id, float x0, float y0, float x1, float y1, uint8_t flip) {
    float coords[20] = {
        x0, -y0, 0.f, 0.f,
        x1, -y0, 1.f, 0.f,
        x0, -y1, 0.f, 1.f,
        x1, -y1, 1.f, 1.f,
    };

    GLuint p = flip ? gv_pos_bgra : gv_pos, c = flip ? gv_coords_bgra : gv_coords;

    glActiveTexture(GL_TEXTURE0);
    glUseProgram(flip ? g_texture_program_bgra : g_texture_program);
    glBindTexture(GL_TEXTURE_2D, id);

    glVertexAttribPointer(p, 2, GL_FLOAT, GL_FALSE, 16, coords);
    glVertexAttribPointer(c, 2, GL_FLOAT, GL_FALSE, 16, &coords[2]);
    glEnableVertexAttribArray(p);
    glEnableVertexAttribArray(c);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); checkGlError();
}

__unused static void draw_cursor(void) {
    float x, y, w, h;

    if (!cursor.width || !cursor.height)
        return;

    x = 2.f * (cursor.x - cursor.xhot) / display.width - 1.f;
    y = 2.f * (cursor.y - cursor.yhot) / display.height - 1.f;
    w = 2.f * cursor.width / display.width;
    h = 2.f * cursor.height / display.height;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw(cursor.id, x, y, x + w, y + h, false);
    glDisable(GL_BLEND);
}
