#include <cstdint>
#include <EGL/egl.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <xkbcommon/xkbcommon-x11.h>

class X11Window {
public:
	X11Window();
	void createWindow(uint32_t width, uint32_t height);
	void hideCursor();
	Display *x_dpy;
	Window window;
	
	EGLDisplay egl_dpy;
	//EGLContext egl_context;
	//EGLSurface surface;

	xcb_connection_t *xcb_connection;
	int32_t keyboard_device_id;
	struct xkb_keymap *keymap;
	struct xkb_state *state;
	Atom  atom1, atom2;
};
