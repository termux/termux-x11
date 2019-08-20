#include <wayland-server.h>
#include "x11-window.hpp"
#include <linux/input.h>
#include <EGL/egl.h>
#include <X11/Xcursor/Xcursor.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <execinfo.h>

#include <thread>
#include <lorie-compositor.hpp>
#include <lorie-egl-helper.hpp>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 480
#define DPI 96

class LorieBackendX11 : public LorieCompositor {
public:
	LorieBackendX11(){};

	void backend_init() override;
	uint32_t input_capabilities() override;
	void swap_buffers() override;
	void get_default_proportions(int32_t* width, int32_t* height) override;
	void get_keymap(int *fd, int *size) override;
	
	void processEvents();
	static int fdListener(int fd, uint32_t mask, void *data);
private:
	LorieEGLHelper helper;
	X11Window win;
	
	void onInit();
	void onUninit();
};

void Backtrace();
void handler(int sig) {
	Backtrace(); 
	return;
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

uint32_t button_convert(uint32_t button) {
	switch (button) {
		case Button1: return BTN_LEFT;
		case Button2: return BTN_MIDDLE;
		case Button3: return BTN_RIGHT;
		default: return BTN_LEFT;
	}
	
	return BTN_LEFT;
}

uint32_t state_convert(uint32_t state) {
	switch (state) {
		case ButtonPress: return WL_POINTER_BUTTON_STATE_PRESSED;
		case ButtonRelease: return WL_POINTER_BUTTON_STATE_RELEASED;
		case KeyPress: return WL_KEYBOARD_KEY_STATE_PRESSED;
		case KeyRelease: return WL_KEYBOARD_KEY_STATE_RELEASED;
		default: return 0;
	}
	return 0;
}

void LorieBackendX11::processEvents() {
	XEvent event;

	while (XPending(win.x_dpy)) {
		XNextEvent (win.x_dpy, &event);
		if (event.type == ConfigureNotify) {
			output_resize(
				event.xconfigure.width, 
				event.xconfigure.height, 
				(int32_t)((event.xconfigure.width*25.4)/DPI), 
				(int32_t)((event.xconfigure.height*25.4)/DPI)
			);
		}
		else if (event.type == Expose) {
			output_redraw();
		}
		else if (event.type == MotionNotify) {
			pointer_motion(event.xbutton.x, event.xbutton.y);
		}
		else if (event.type == ButtonPress || event.type == ButtonRelease) {
			pointer_button(button_convert(event.xbutton.button), state_convert(event.type));
		}
		else if (event.type == KeyPress || event.type == KeyRelease) {
			keyboard_key(event.xkey.keycode - 8, state_convert(event.type));
			xkb_state_update_key (win.state, event.xkey.keycode, (event.type == KeyPress) ? XKB_KEY_DOWN : XKB_KEY_UP);
			//update_modifiers (b, seat);
		}
		else if (event.type == FocusIn) {
			xkb_state_unref (win.state);
			win.state = xkb_x11_state_new_from_device (win.keymap, win.xcb_connection, win.keyboard_device_id);
			//update_modifiers (b, seat);
		}
		else if (event.type == ClientMessage) {
		if(event.xclient.message_type == win.atom1 &&
			(Atom) event.xclient.data.l[0] == win.atom2) {
				terminate();
			}
		}
		else if (event.type == DestroyNotify) {
			terminate();
		}
	}
}

static void proc(int fd, uint32_t mask, void *data) {
	LorieBackendX11* that = static_cast<LorieBackendX11*>(data);

	that->processEvents();
}

void LorieBackendX11::onInit() {
	renderer.init();
}

void LorieBackendX11::onUninit() {
	renderer.uninit();
}

void LorieBackendX11::backend_init() {
	win.createWindow (WINDOW_WIDTH, WINDOW_HEIGHT);
	helper.init(win.x_dpy);
	
	helper.onInit = std::bind(std::mem_fn(&LorieBackendX11::onInit), this);
	helper.onUninit = std::bind(std::mem_fn(&LorieBackendX11::onUninit), this);
	
	helper.setWindow(win.window);
	renderer.resize(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_WIDTH/DPI*25.4, WINDOW_HEIGHT/DPI*25.4);
	
	wl_event_loop_add_fd(
		wl_display_get_event_loop(display), 
		XConnectionNumber(win.x_dpy), 
		WL_EVENT_READABLE, 
		(wl_event_loop_fd_func_t)
			&proc, 
		this
	);
}

uint32_t LorieBackendX11::input_capabilities() {
	return WL_SEAT_CAPABILITY_POINTER|WL_SEAT_CAPABILITY_KEYBOARD;
}

void LorieBackendX11::get_default_proportions(int32_t* width, int32_t* height) {
	if (!width || !height) return;
	*width = WINDOW_WIDTH;
	*height = WINDOW_HEIGHT;
}

void LorieBackendX11::swap_buffers() {
	//eglSwapBuffers (win.egl_dpy, win.surface);
	helper.swap();
}

void LorieBackendX11::get_keymap (int *fd, int *size) {
	char *string = xkb_keymap_get_as_string (win.keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
	*size = strlen (string) + 1;
	char *xdg_runtime_dir = getenv ("XDG_RUNTIME_DIR");
	*fd = open (xdg_runtime_dir, O_TMPFILE|O_RDWR|O_EXCL, 0600);
	ftruncate (*fd, *size);
	char *map = (char*) mmap (NULL, *size, PROT_READ|PROT_WRITE, MAP_SHARED, *fd, 0);
	strcpy (map, string);
	munmap (map, *size);
	free (string);
}

int main(int argc, char *argv[]) {
	signal(SIGSEGV, handler);
	signal(SIGABRT, handler);

	LorieBackendX11 b;
	b.start();
	return 0;
}
