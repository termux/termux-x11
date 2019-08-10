#include <EGL/egl.h>
#include <EGL/eglext.h>

#if defined(__ANDROID__)
#define EGL_NO_WINDOW nullptr
#else
#define EGL_NO_WINDOW 0
#endif

class LorieEGLHelper {
public:
	EGLDisplay dpy = EGL_NO_DISPLAY;
	EGLConfig cfg;
	
	EGLSurface sfc = EGL_NO_SURFACE;
	EGLContext ctx = EGL_NO_CONTEXT;
	EGLNativeWindowType win = EGL_NO_WINDOW;
	
	bool init(EGLNativeDisplayType display);
	bool setWindow(EGLNativeWindowType window);
	void swap();
	void uninit();
};
