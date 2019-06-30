#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-isolate-declaration"
#pragma ide diagnostic ignored "cppcoreguidelines-avoid-magic-numbers"
#include <unistd.h>
#include <jni.h>
#include <android/native_window_jni.h>
#include <pthread.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <wayland-server.h>
#include "backend.h"
#include "log.h"

static void checkEGLError(int line) {
    char* error = NULL;
    switch(eglGetError()) {
#define E(code, desc) case code: error = desc; break;
        case EGL_SUCCESS: return; // "No error"
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
        default: error = "Unknown error";
    }
    LOGE("EGL: %s after %d", error, line);
}
#define checkEGLError() checkEGLError(__LINE__)

enum {
	ACTION_KEY = 1,
	ACTION_POINTER = 2,
	ACTION_WIN_CHANGE = 3,
	#ifdef __ANDROID__
	ACTION_JNI_WIN_CHANGE = 4,
	#endif
};
static const EGLint ctxattr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,  // Request opengl ES2.0
		EGL_NONE
};

static const EGLint sfcattr[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
};

typedef struct {
	uint8_t type;
	union {
		struct {
			uint8_t state;
			uint16_t key;
		} key;
		struct {
			uint16_t state, button;
            uint32_t x, y;
		} pointer;
		struct {
			EGLDisplay dpy;
			EGLNativeWindowType win;
			uint32_t width, height;
		} window_change;
		#ifdef __ANDROID__
		struct {
			JavaVM* jvm;
			jobject surface;
			uint32_t width, height;
		} jni_window_change;
		#endif
	};
} lorie_event;

struct backend_android {
	EGLDisplay dpy;
	EGLContext ctx;
	EGLNativeDisplayType win;
	EGLConfig config;
	EGLSurface sfc;

	struct callbacks callbacks;

	int fds[2];
};

struct backend_android *backend = NULL;

void lorie_key_event(void __unused *b, uint8_t state, uint16_t key) {
	if (!backend) return;
	
	lorie_event e;
	memset(&e, 0, sizeof(lorie_event));
	
	e.type = ACTION_KEY;
	e.key.state = state;
	e.key.key = key;
	
	write(backend->fds[0], &e, sizeof(lorie_event));
}

void lorie_pointer_event(void __unused *b, uint8_t state, uint16_t button, uint32_t x, uint32_t y) {
	if (!backend) return;
	
	lorie_event e;
	memset(&e, 0, sizeof(lorie_event));
	
	e.type = ACTION_POINTER;
	e.pointer.state = state;
	e.pointer.button = button;
	e.pointer.x = x;
	e.pointer.y = y;
	
	write(backend->fds[0], &e, sizeof(lorie_event));
}

void lorie_window_change_event(void __unused *b, EGLDisplay dpy, EGLNativeWindowType win, uint32_t width, uint32_t height) {
	if (!backend) return;
	
	lorie_event e;
	memset(&e, 0, sizeof(lorie_event));
	
	e.type = ACTION_WIN_CHANGE;
	e.window_change.dpy = dpy;
	e.window_change.win = win;
	e.window_change.width = width;
	e.window_change.height = height;
	
	write(backend->fds[0], &e, sizeof(lorie_event));
}

void lorie_jni_window_change_event(void __unused *b, JavaVM* jvm, jobject surface, uint32_t width, uint32_t height) {
	if (!backend) return;
	
	lorie_event e;
	memset(&e, 0, sizeof(lorie_event));
	
	e.type = ACTION_JNI_WIN_CHANGE;
	e.jni_window_change.jvm = jvm;
	e.jni_window_change.surface = surface;
	e.jni_window_change.width = width;
	e.jni_window_change.height = height;
	
	write(backend->fds[0], &e, sizeof(lorie_event));
}

static int has_data(int socket) {
	int count = 0;
	ioctl(socket, FIONREAD, &count);
	return count;
}

JNIEXPORT jlong JNICALL
Java_com_termux_wtermux_LorieService_createLorieThread(JNIEnv __unused *env, jobject __unused instance) {
	pthread_t thread;
	//void *server = NULL;
	
	setenv("XDG_RUNTIME_DIR", "/data/data/com.termux/files/usr/tmp", 1);
	pthread_create(&thread, NULL, (void *(*)(void *))lorie_start, NULL);
	while(backend == NULL) { usleep(10); }
	
	//return (jlong) compositor;
	return 1;
}

JNIEXPORT void JNICALL
Java_com_termux_wtermux_LorieService_windowChanged(JNIEnv *env, jobject __unused instance, jlong __unused jcompositor, jobject jsurface, jint width, jint height) {
	if (backend == NULL) return;
#if 1
	EGLNativeWindowType win = ANativeWindow_fromSurface(env, jsurface);
	if (win == NULL) {
		LOGE("Surface is invalid");
		lorie_window_change_event(backend, NULL, NULL, (uint32_t) width, (uint32_t) height);
		return;
	} 
	
	lorie_window_change_event(backend, NULL, win, (uint32_t) width,
							  (uint32_t) height);
#else
	JavaVM* jvm;
	(*env)->GetJavaVM(env, &jvm);
	if (jvm == NULL) {
		LOGE("Error getting jvm instance");
		return;
	}
	
	jobject surface = (*env)->NewGlobalRef(env, jsurface);
	if (surface == NULL) {
		LOGE("Error creating global reference of Surface object");
		return;
	}
	
	lorie_jni_window_change_event(backend, jvm, surface, width, height);
#endif
}

JNIEXPORT void JNICALL
Java_com_termux_wtermux_LorieService_onTouch(JNIEnv __unused *env, jobject __unused instance, jlong __unused jcompositor, jint type, jint button, jint x, jint y) {
	if (backend == NULL) return;
	lorie_pointer_event(backend, (uint8_t) type, (uint16_t) button, (uint32_t) x, (uint32_t) y);
}

JNIEXPORT void JNICALL
Java_com_termux_wtermux_LorieService_onKey(JNIEnv __unused *env, jobject __unused instance, jlong __unused compositor,
										   jint type, jint key) {

	lorie_key_event(backend, (uint8_t) type, (uint16_t) key);

}

////////////////////////////////////////////////////////////////////////

void backend_init (struct callbacks *callbacks) {
	backend = calloc (1, sizeof(struct backend_android));
	if (backend == NULL) {
		LOGE("Can not allocate backend_android");
		return;
	}
	
	backend->callbacks = *callbacks;

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, backend->fds)) {

	}
	
	// Setup EGL
	EGLint num_configs_returned;
	backend->dpy = eglGetDisplay (EGL_DEFAULT_DISPLAY); checkEGLError();
	eglInitialize (backend->dpy, NULL, NULL); checkEGLError();
	eglBindAPI (EGL_OPENGL_ES_API);

	eglChooseConfig (backend->dpy, sfcattr, &backend->config, 1, &num_configs_returned); checkEGLError();
	backend->ctx = eglCreateContext (backend->dpy, backend->config, EGL_NO_CONTEXT, ctxattr); checkEGLError();
    eglMakeCurrent(backend->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, backend->ctx); checkEGLError();
}

EGLDisplay __unused backend_get_egl_display (void) {
	return backend->dpy;
}

void backend_swap_buffers (void) {
	eglSwapBuffers(backend->dpy, backend->sfc); checkEGLError();
}

void backend_dispatch_nonblocking (void)
{	
	lorie_event ev;

	while (has_data(backend->fds[1]) >= (int) sizeof(lorie_event)) {
		read(backend->fds[1], &ev, sizeof(lorie_event));
		switch (ev.type) {
			case ACTION_KEY:
				if (backend->callbacks.key)
				    backend->callbacks.key(ev.key.key, ev.key.state);
                break;
            case ACTION_POINTER:
                if (backend->callbacks.mouse_motion)
				    backend->callbacks.mouse_motion(ev.pointer.x, ev.pointer.y);

				if (ev.pointer.state != WL_POINTER_MOTION) {
					if (backend->callbacks.mouse_button)
					backend->callbacks.mouse_button(ev.pointer.button, ev.pointer.state);
				}
				break;
			case ACTION_WIN_CHANGE:
				if (ev.window_change.win != backend->win) {
				    if (backend->sfc)
				        eglDestroySurface(backend->dpy, backend->sfc);
				    backend->sfc = eglCreateWindowSurface(backend->dpy, backend->config, ev.window_change.win, NULL); checkEGLError();
				    backend->win = ev.window_change.win;
				}
				checkEGLError();
				eglMakeCurrent(backend->dpy, backend->sfc, backend->sfc, backend->ctx); checkEGLError();

				if (backend->callbacks.resize)
				    backend->callbacks.resize(ev.window_change.width, ev.window_change.height);
				break;
			#if 0
			#ifdef __ANDROID__
			case ACTION_JNI_WIN_CHANGE:
			{
				JavaVM* jvm = ev.jni_window_change.jvm;
				JNIEnv* env;
				(*jvm)->AttachCurrentThread(jvm, (void**)&env, NULL);
				if (!jvm || !env) {
					weston_log("Failed getting JNIEnv\n");
					return -1;
				}
				
				b->edpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
				b->ewin = ANativeWindow_fromSurface(env, ev.jni_window_change.surface);
				if (b->ewin == NULL) {
					weston_log("Error getting EGLNativeWindowType from Java Surface object\n");
					return -1;
				}
				
				struct weston_mode mode = b->output->mode;

				if (mode.width == ev.jni_window_change.width &&
					mode.height == ev.jni_window_change.height)
					break;

				mode.width = ev.jni_window_change.width;
				mode.height = ev.jni_window_change.height;

				if (weston_output_mode_set_native(&b->output->base,
								  &mode, b->output->scale) < 0)
					weston_log("Mode switch failed\n");

				break;
			}
			#endif
			#endif
			default:break;
		}
	}
}

void backend_wait_for_events (int wayland_fd) {
	if (!backend) return;
	struct pollfd fds[2] = {{backend->fds[1], POLLIN}, {wayland_fd, POLLIN}};
	poll (fds, 2, -1);
}

void __unused backend_get_keymap (int __unused *fd, int __unused *size) {
	
}

long backend_get_timestamp (void) {
	struct timespec t;
	clock_gettime (CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

void backend_get_dimensions(uint32_t *width, uint32_t *height) {
	if (width) *width = 480;
	if (height) *height = 800;
}
#pragma clang diagnostic pop