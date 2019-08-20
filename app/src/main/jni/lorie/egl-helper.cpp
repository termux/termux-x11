#include <log.h>
#include <lorie-egl-helper.hpp>
#include <GLES2/gl2.h>

static char* eglGetErrorText(int code) {
		switch(eglGetError()) {
#define E(code, desc) case code: return (char*) desc; break;
				case EGL_SUCCESS: return nullptr; // "No error"
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
				default: return (char*) "Unknown error";
		}
}

#undef checkEGLError
void checkEGLError(int line) {
	char *error = eglGetErrorText(eglGetError());
	if (error != nullptr)
		LOGE("EGL: %s after %d", error, line);
}
#define checkEGLError() checkEGLError(__LINE__)

bool LorieEGLHelper::init(EGLNativeDisplayType display) {
	EGLint majorVersion, minorVersion;
	
	LOGV("Initializing EGL");
	dpy = eglGetDisplay(display); checkEGLError();
	if (dpy == EGL_NO_DISPLAY) {
		LOGE("LorieEGLHelper::init : eglGetDisplay failed: %s", eglGetErrorText(eglGetError()));
		return false;
	}
	
	if (eglInitialize(dpy, &majorVersion, &minorVersion) != EGL_TRUE) {
		LOGE("LorieEGLHelper::init : eglInitialize failed: %s", eglGetErrorText(eglGetError()));
		return false;
	}
	
	const EGLint configAttribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_NONE
	};

	EGLint numConfigs;

	if (eglChooseConfig(dpy, configAttribs, &cfg, 1, &numConfigs) != EGL_TRUE) {
		LOGE("LorieEGLHelper::init : eglChooseConfig failed: %s", eglGetErrorText(eglGetError()));
		return false;
	}

	eglBindAPI(EGL_OPENGL_ES_API);
	
	const EGLint ctxattribs[] = {
		EGL_CONTEXT_CLIENT_VERSION,
		2,	// Request opengl ES2.0
		EGL_NONE
	};
	
	ctx = eglCreateContext(dpy, cfg, NULL, ctxattribs); 
	if (ctx == EGL_NO_CONTEXT) {
		LOGE("LorieEGLHelper::init : eglCreateContext failed: %s", eglGetErrorText(eglGetError()));
		return false;
	}
	
	if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONFIG_KHR) != EGL_TRUE) {
		LOGE("LorieEGLHelper::init : eglMakeCurrent failed: %s", eglGetErrorText(eglGetError()));
		return false;
	}
	
	return true;
}

bool LorieEGLHelper::setWindow(EGLNativeWindowType window) {
    LOGV("Trying to use window %p", window);
	if (sfc != EGL_NO_SURFACE) {
		if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE) {
			LOGE("LorieEGLHelper::setWindow : eglMakeCurrent (EGL_NO_SURFACE) failed: %s", eglGetErrorText(eglGetError()));
			return false;
		}
		if (eglDestroySurface(dpy, sfc) != EGL_TRUE) {
			LOGE("LorieEGLHelper::setWindow : eglDestoySurface failed: %s", eglGetErrorText(eglGetError()));
			return false;
		}
	};
	
	sfc = EGL_NO_SURFACE;
	win = window;

	if (win == EGL_NO_WINDOW) {
		if (onUninit != nullptr) onUninit();
        return true;
    }
	
	sfc = eglCreateWindowSurface(dpy, cfg, win, NULL);
	if (sfc == EGL_NO_SURFACE) {
		LOGE("LorieEGLHelper::setWindow : eglCreateWindowSurface failed: %s", eglGetErrorText(eglGetError()));
		if (onUninit != nullptr) onUninit();
		return false;
	}
	
	if (eglMakeCurrent(dpy, sfc, sfc, ctx) != EGL_TRUE) {
		LOGE("LorieEGLHelper::setWindow : eglMakeCurrent failed: %s", eglGetErrorText(eglGetError()));
		if (onUninit != nullptr) onUninit();
		return false;
	}

	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);

	swap();

	if (onInit != nullptr)
	    onInit();
	
	return true;
}

void LorieEGLHelper::swap() {
	EGLint b = eglSwapBuffers(dpy, sfc);
	if (b != EGL_TRUE) {
		EGLint err = eglGetError();
		if (err == EGL_BAD_SURFACE) {
			LOGE("eglSwapBuffers failed because of invalid surface. Regenerating surface.");
			setWindow(win);
		} else if (err == EGL_BAD_CONTEXT || err == EGL_CONTEXT_LOST) {
			LOGE("eglSwapBuffers failed because of invalid context. Regenerating context.");
			const EGLint ctxattribs[] = {
				EGL_CONTEXT_CLIENT_VERSION,
				2,	// Request opengl ES2.0
				EGL_NONE
			};
			
			ctx = eglCreateContext(dpy, cfg, NULL, ctxattribs); 
			if (ctx == EGL_NO_CONTEXT) {
				LOGE("LorieEGLHelper::init : eglCreateContext failed: %s", eglGetErrorText(eglGetError()));
				return;
			}
		}
	}
}

void LorieEGLHelper::uninit() {
	if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE) {
		LOGE("LorieEGLHelper::uninit : eglMakeCurrent failed: %s", eglGetErrorText(eglGetError()));
		return;
	}
	if (eglDestroyContext(dpy, ctx) != EGL_TRUE) {
		LOGE("LorieEGLHelper::uninit : eglDestroyContext failed: %s", eglGetErrorText(eglGetError()));
		return;
	}
	ctx = EGL_NO_CONTEXT;
	
	if (eglDestroySurface(dpy, sfc) != EGL_TRUE) {
		LOGE("LorieEGLHelper::uninit : eglDestroySurface failed: %s", eglGetErrorText(eglGetError()));
		return;
	}
	sfc = EGL_NO_SURFACE;
	
	if (eglTerminate(dpy) != EGL_TRUE) {
		LOGE("LorieEGLHelper::uninit : eglTerminate failed: %s", eglGetErrorText(eglGetError()));
		return;
	}
	dpy = EGL_NO_DISPLAY;
}
