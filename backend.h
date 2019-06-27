#include <EGL/egl.h>
#include <GLES2/gl2.h>

struct modifier_state {
	uint32_t depressed, latched, locked, group;
};

struct callbacks {
	void (*resize) (int width, int height);
	void (*draw) (void);
	void (*mouse_motion) (int x, int y);
	void (*mouse_button) (int button, int state);
	void (*key) (int key, int state);
	void (*modifiers) (struct modifier_state modifier_state);
	void (*terminate) ();
};

void backend_init (struct callbacks *callbacks);
EGLDisplay backend_get_egl_display (void);
void backend_swap_buffers (void);
void backend_dispatch_nonblocking (void);
void backend_wait_for_events (int wayland_fd);
void backend_get_keymap (int *fd, int *size);
long backend_get_timestamp (void);

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * data);
