#include "x11-window.hpp"


X11Window::X11Window() {
	x_dpy = XOpenDisplay (NULL);
	xcb_connection = XGetXCBConnection (x_dpy);
	struct xkb_context *context = xkb_context_new (XKB_CONTEXT_NO_FLAGS);
	xkb_x11_setup_xkb_extension (xcb_connection, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION, (xkb_x11_setup_xkb_extension_flags) 0, NULL, NULL, NULL, NULL);
	keyboard_device_id = xkb_x11_get_core_keyboard_device_id (xcb_connection);
	keymap = xkb_x11_keymap_new_from_device (context, xcb_connection, keyboard_device_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
	state = xkb_x11_state_new_from_device (keymap, xcb_connection, keyboard_device_id);
	
	egl_dpy = eglGetDisplay (x_dpy);
	eglInitialize (egl_dpy, NULL, NULL);
}

void X11Window::hideCursor() {
	XColor dummy;
	char data[1] = {0};
	Pixmap blank = XCreateBitmapFromData(x_dpy, window, data, 1, 1);
	if (blank == None) printf("out of memory.\n");
	Cursor cursor = XCreatePixmapCursor(x_dpy, blank, blank, &dummy, &dummy, 0, 0);
	XFreePixmap(x_dpy, blank);
	XDefineCursor (x_dpy, window, cursor);
}

void X11Window::createWindow(uint32_t width, uint32_t height) {
	// setup EGL
	//static EGLint ctxattr[] = {
	//	EGL_CONTEXT_CLIENT_VERSION, 2,  // Request opengl ES2.0
	//	EGL_NONE
	//};
	
	static const EGLint attribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};
	EGLConfig config;
	EGLint num_configs_returned;
	eglChooseConfig (egl_dpy, attribs, &config, 1, &num_configs_returned);
	
	// get the visual from the EGL config
	EGLint visual_id;
	eglGetConfigAttrib (egl_dpy, config, EGL_NATIVE_VISUAL_ID, &visual_id);
	XVisualInfo visual_template;
	visual_template.visualid = visual_id;
	int num_visuals_returned;
	XVisualInfo *visual = XGetVisualInfo (x_dpy, VisualIDMask, &visual_template, &num_visuals_returned);
	
	// create a window
	XSetWindowAttributes window_attributes;
	window_attributes.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask;
	window_attributes.colormap = XCreateColormap (x_dpy, RootWindow(x_dpy,DefaultScreen(x_dpy)), visual->visual, AllocNone);
	window = XCreateWindow (
		x_dpy,
		RootWindow(x_dpy, DefaultScreen(x_dpy)),
		0, 0,
		width, height,
		0, // border width
		visual->depth, // depth
		InputOutput, // class
		visual->visual, // visual
		CWEventMask|CWColormap, // attribute mask
		&window_attributes // attributes
	);
	
    atom1 = XInternAtom(x_dpy, "WM_PROTOCOLS", 0);
    atom2 = XInternAtom(x_dpy, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(x_dpy, window, &atom2, 1);
	
	// EGL context and surface	
	//eglBindAPI (EGL_OPENGL_ES_API);
	//egl_context = eglCreateContext (egl_dpy, config, EGL_NO_CONTEXT, ctxattr);
	//surface = eglCreateWindowSurface (egl_dpy, config, window, NULL);
	//eglMakeCurrent (egl_dpy, surface, surface, egl_context);
	
	XFree (visual);
	
	XMapWindow (x_dpy, window);
	hideCursor();
}
