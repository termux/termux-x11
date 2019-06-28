#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <wayland-server.h>
#include <dlfcn.h>
#include "backend.h"
#include "renderer.h"
#include "log.h"

bool trace_funcs = false;

void __attribute__((no_instrument_function))
__cyg_profile_func_enter (void *func,  void *caller) {
  if (!trace_funcs) return;
  Dl_info info;
  if (dladdr(func, &info))
    LOGD ("enter %p [%s] %s\n", func, (info.dli_fname) ? info.dli_fname : "?", info.dli_sname ? info.dli_sname : "?");
}
void __attribute__((no_instrument_function))
__cyg_profile_func_exit (void *func,  void *caller) {
  if (!trace_funcs) return;
  Dl_info info;
  if (dladdr(func, &info))
    LOGD ("leave %p [%s] %s\n", func, (info.dli_fname) ? info.dli_fname : "?", info.dli_sname ? info.dli_sname : "?");
}

#define static // backtrace do not report static function names

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

#define BINDFUNC(interface, version, implementation) \
	static void interface##_bind (struct wl_client *client, void *data, uint32_t _version, uint32_t id) { \
		LOGD("bind: " #interface); \
		struct wl_resource *resource = wl_resource_create (client, &interface##_interface, version, id); \
		if (resource == NULL) { \
			wl_client_post_no_memory(client); \
			return; \
		} \
		wl_resource_set_implementation (resource, &implementation, NULL, NULL); \
	}

struct lorie_composiror {
	struct wl_display *display;
	struct wl_resource *wl_output;
	struct wl_list clients;
	struct wl_list surfaces;
	struct wl_list shell_surfaces;
	
	struct surface *toplevel;
	struct surface *cursor;
	struct modifier_state modifier_state;
	
	bool running;
	bool redraw_needed;
	
	uint32_t width, height;
	
	struct renderer* renderer;
} c = {0};

struct client {
	struct wl_client *client;
	struct wl_resource *pointer;
	struct wl_resource *keyboard;
	struct wl_list link;
};

static struct client *get_client (struct wl_client *_client) {
	struct client *client;
	wl_list_for_each (client, &c.clients, link) {
		if (client->client == _client) return client;
	}
	client = calloc (1, sizeof(struct client));
	client->client = _client;
	wl_list_insert (&c.clients, &client->link);
	return client;
}

struct surface {
	struct wl_resource *surface;
	//struct wl_resource *xdg_surface;
	struct wl_resource *buffer;
	struct wl_resource *frame_callback;
	//int x, y;
	struct texture *texture;
	struct client *client;
	struct shell_surface *shell_surface;
	struct wl_list link;
	uint32_t width, height;
	bool entered;
};

struct shell_surface {
	struct wl_resource *shell_surface;
	struct surface* surface;
	struct wl_list link;
};

struct lorie_client {
	struct wl_client *client;
	struct wl_resource *pointer;
	struct wl_resource *keyboard;
	struct wl_list link;
};

void *null = NULL;

// region
/*No op*/ static void region_destroy (struct wl_client *client, struct wl_resource *resource) {}
/*No op*/ static void region_add (struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {}
/*No op*/ static void region_subtract (struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {}
/*No op*/ static struct wl_region_interface region_interface = {&region_destroy, &region_add, &region_subtract};

// surface
/*No op*/ static void surface_destroy (struct wl_client *client, struct wl_resource *resource) {}
static void surface_attach (struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer, int32_t x, int32_t y) {
	struct surface *surface = wl_resource_get_user_data (resource);
	surface->buffer = buffer;
	
	struct wl_shm_buffer *shm_buffer = wl_shm_buffer_get (buffer);
	if (shm_buffer == NULL) {
		surface->width = surface->height = 0;
		return;
	}
	surface->width = wl_shm_buffer_get_width (shm_buffer);
	surface->height = wl_shm_buffer_get_height (shm_buffer);
	
	if (!surface->texture || 
		surface->texture->width != surface->width || 
		surface->texture->height != surface->height) {
		texture_destroy(&surface->texture);
		surface->texture = texture_create(surface->width, surface->height);
	}
}

static void surface_damage (struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	c.redraw_needed = true;
	struct surface *surface = wl_resource_get_user_data (resource);
	struct texture* texture;
	if (!surface || !surface->texture) {
		texture = surface->texture;
		texture->damage.x = min(texture->damage.x, x);
		texture->damage.y = min(texture->damage.y, y);
		texture->damage.width = max(texture->damage.width, width);
		texture->damage.height = max(texture->damage.height, height);
	}
}
static void surface_frame (struct wl_client *client, struct wl_resource *resource, uint32_t callback) {
	struct surface *surface = wl_resource_get_user_data (resource);
	surface->frame_callback = wl_resource_create (client, &wl_callback_interface, 1, callback);
}
/*No op*/ static void surface_set_opaque_region (struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {}
/*No op*/ static void surface_set_input_region (struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {}
static void surface_commit (struct wl_client *client, struct wl_resource *resource) {
	struct surface *surface = wl_resource_get_user_data (resource);
	if (!surface || !surface->buffer) return;
	struct wl_shm_buffer *shm_buffer = wl_shm_buffer_get (surface->buffer);
	if (surface->texture && shm_buffer) {
		void *data = wl_shm_buffer_get_data (shm_buffer);
		if (data) {
			texture_upload(surface->texture, data);
		}
	}
	wl_buffer_send_release (surface->buffer);
		
	if (surface->frame_callback) {
		wl_callback_send_done (surface->frame_callback, backend_get_timestamp());
		wl_resource_destroy (surface->frame_callback);
		surface->frame_callback = NULL;
	}
}
/*No op*/ static void surface_set_buffer_transform (struct wl_client *client, struct wl_resource *resource, int32_t transform) {}
/*No op*/ static void surface_set_buffer_scale (struct wl_client *client, struct wl_resource *resource, int32_t scale) {}
static void surface_delete (struct wl_resource *resource) {
	struct surface *surface = wl_resource_get_user_data (resource);
	if (c.toplevel == surface) c.toplevel = NULL;
	wl_list_remove (&surface->link);
	texture_destroy(&surface->texture);
	free (surface);
}
static struct wl_surface_interface surface_interface = {&surface_destroy, &surface_attach, &surface_damage, &surface_frame, &surface_set_opaque_region, &surface_set_input_region, &surface_commit, &surface_set_buffer_transform, &surface_set_buffer_scale};

// compositor
static void compositor_create_surface (struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	struct surface *surface = calloc (1, sizeof(struct surface));
	surface->surface = wl_resource_create (client, &wl_surface_interface, 4, id);
	wl_resource_set_implementation (surface->surface, &surface_interface, surface, &surface_delete);
	surface->client = get_client (client);
	wl_list_insert (&c.surfaces, &surface->link);
}
static void compositor_create_region (struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	struct wl_resource *region = wl_resource_create (client, &wl_region_interface, 1, id);
	wl_resource_set_implementation (region, &region_interface, NULL, NULL);
}
static struct wl_compositor_interface compositor_interface = {&compositor_create_surface, &compositor_create_region};
BINDFUNC(wl_compositor, 4, compositor_interface);


// shell surface
/*No op*/ static void shell_surface_pong (struct wl_client *client, struct wl_resource *resource, uint32_t serial) {}
/*No op*/ static void shell_surface_move (struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial) {}
/*No op*/ static void shell_surface_resize (struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, uint32_t edges) {}	       
static void shell_surface_set_toplevel (struct wl_client *client, struct wl_resource *resource) {
	struct shell_surface *shell_surface = wl_resource_get_user_data (resource);
	c.toplevel = shell_surface->surface;
	wl_shell_surface_send_configure(shell_surface->shell_surface, 0, c.width, c.height);
}
/*No op*/ static void shell_surface_set_transient (struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags) {}
/*No op*/ static void shell_surface_set_fullscreen (struct wl_client *client, struct wl_resource *resource, uint32_t method, uint32_t framerate, struct wl_resource *output) {}
/*No op*/ static void shell_surface_set_popup (struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags) {}
/*No op*/ static void shell_surface_set_maximized (struct wl_client *client, struct wl_resource *resource, struct wl_resource *output) {}
/*No op*/ static void shell_surface_set_title (struct wl_client *client, struct wl_resource *resource, const char *title) {}
/*No op*/ static void shell_surface_set_class (struct wl_client *client, struct wl_resource *resource, const char *class_) {}
static struct wl_shell_surface_interface shell_surface_interface = {&shell_surface_pong, &shell_surface_move, &shell_surface_resize, &shell_surface_set_toplevel, &shell_surface_set_transient, &shell_surface_set_fullscreen, &shell_surface_set_popup, &shell_surface_set_maximized, &shell_surface_set_title, &shell_surface_set_class};

// wl shell
static void shell_get_shell_surface (struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *res_surface) {
	struct shell_surface *shell_surface = calloc (1, sizeof(struct surface));
	struct surface *surface = wl_resource_get_user_data (res_surface);
	shell_surface->shell_surface = wl_resource_create (client, &wl_shell_surface_interface, 1, id);
	wl_resource_set_implementation (shell_surface->shell_surface, &shell_surface_interface, shell_surface, NULL);
	shell_surface->surface = surface;
	surface->shell_surface = shell_surface;
}
static struct wl_shell_interface shell_interface = {&shell_get_shell_surface};
BINDFUNC(wl_shell, 1, shell_interface);

// pointer
static void pointer_set_cursor (struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *_surface, int32_t hotspot_x, int32_t hotspot_y) {
	//struct surface *surface = wl_resource_get_user_data (_surface);
	//cursor = surface;
}
/*No op*/ static void pointer_release (struct wl_client *client, struct wl_resource *resource) {}
/*No op*/ static struct wl_pointer_interface pointer_interface = {&pointer_set_cursor, &pointer_release};

// keyboard
/*No op*/ static void keyboard_release (struct wl_client *client, struct wl_resource *resource) {}
/*No op*/ static struct wl_keyboard_interface keyboard_interface = {&keyboard_release};

// seat
static void seat_get_pointer (struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	struct wl_resource *pointer = wl_resource_create (client, &wl_pointer_interface, 7, id);
	wl_resource_set_implementation (pointer, &pointer_interface, NULL, NULL);
	get_client(client)->pointer = pointer;
}
static void seat_get_keyboard (struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	struct wl_resource *keyboard = wl_resource_create (client, &wl_keyboard_interface, 7, id);
	wl_resource_set_implementation (keyboard, &keyboard_interface, NULL, NULL);
	//get_client(client)->keyboard = keyboard;
	//int fd, size;
	//backend_get_keymap (&fd, &size);
	//wl_keyboard_send_keymap (keyboard, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd, size);
	//close (fd);
}
/*No op*/ static void seat_get_touch (struct wl_client *client, struct wl_resource *resource, uint32_t id) {}
static struct wl_seat_interface seat_interface = {&seat_get_pointer, &seat_get_keyboard, &seat_get_touch};
static void wl_seat_bind (struct wl_client *client, void *data, uint32_t version, uint32_t id) {
	LOGD("bind: wl_seat");
	struct wl_resource *seat = wl_resource_create (client, &wl_seat_interface, 7, id);
	wl_resource_set_implementation (seat, &seat_interface, NULL, NULL);
	wl_seat_send_capabilities (seat, WL_SEAT_CAPABILITY_POINTER|WL_SEAT_CAPABILITY_KEYBOARD);
}

static void
output_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}
static const struct wl_output_interface output_interface = {
	output_release,
};

static void
wl_output_unbind (struct wl_resource *resource) {
	if (c.wl_output == resource)
		c.wl_output = NULL;
}

static void
wl_output_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	int DPI = 240;
	struct wl_resource *resource = wl_resource_create(client, &wl_output_interface, 3, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &output_interface, NULL, &wl_output_unbind);
	
	int mm_width = c.width/DPI*25*2;
	int mm_height = c.height/DPI*25*2;
	wl_output_send_geometry(resource, 0, 0, mm_width, mm_height, 0, "non-weston", "none", 0);
	if (version >= WL_OUTPUT_SCALE_SINCE_VERSION)
		wl_output_send_scale(resource, 1.0);
	wl_output_send_mode(resource, 3, c.width, c.height, 60000);

	if (version >= WL_OUTPUT_DONE_SINCE_VERSION)
		wl_output_send_done(resource);
	
	c.wl_output = resource;
}

// backend callbacks
static void handle_resize_event (int width, int height) {
	//glViewport(0, 0, width, height);
	c.width = width;
	c.height = height;	
	if (c.wl_output) {
		wl_output_send_mode(c.wl_output, 3, width, height, 60000);
		wl_output_send_done(c.wl_output);
		if (c.toplevel && c.toplevel->client->pointer) {
			wl_pointer_send_leave(c.toplevel->client->pointer, 0, c.toplevel->surface);
			wl_pointer_send_enter (c.toplevel->client->pointer, 0, c.toplevel->surface, 0, 0);
		}
		
		if (c.toplevel && c.toplevel->shell_surface) {
			wl_shell_surface_send_configure(c.toplevel->shell_surface->shell_surface, 0, width, height);
			wl_shell_surface_send_ping(c.toplevel->shell_surface->shell_surface, 33231);
		}
	}
}
static void handle_draw_event (void) {
	c.redraw_needed = true;
}
static void handle_mouse_motion_event (int x, int y) {
	if (!c.toplevel || !c.toplevel->client->pointer) return;
	wl_fixed_t surface_x = wl_fixed_from_double (x);
	wl_fixed_t surface_y = wl_fixed_from_double (y);
	if (!c.toplevel->entered) {
		wl_pointer_send_enter (c.toplevel->client->pointer, 0, c.toplevel->surface, surface_x, surface_y);
		c.toplevel->entered = true;
	}
	wl_pointer_send_motion (c.toplevel->client->pointer, backend_get_timestamp(), surface_x, surface_y);
	
	if (wl_resource_get_version(c.toplevel->client->pointer) >=
	    WL_POINTER_FRAME_SINCE_VERSION) {
		wl_pointer_send_frame(c.toplevel->client->pointer);
	}
}
static void handle_mouse_button_event (int button, int state) {
	if (!c.toplevel || !c.toplevel->client->pointer) return;
	wl_pointer_send_button (c.toplevel->client->pointer, 0, backend_get_timestamp(), button, state);
	if (wl_resource_get_version(c.toplevel->client->pointer) >=
	    WL_POINTER_FRAME_SINCE_VERSION) {
		wl_pointer_send_frame(c.toplevel->client->pointer);
	}
}
static void handle_key_event (int key, int state) {
	//if (!c.toplevel || !c.toplevel->client->keyboard) return;
	//wl_keyboard_send_key (c.toplevel->client->keyboard, 0, backend_get_timestamp(), key, state);
}
static void handle_modifiers_changed (struct modifier_state new_state) {
	//if (new_state.depressed == c.modifier_state.depressed && new_state.latched == c.modifier_state.latched && new_state.locked == c.modifier_state.locked && new_state.group == c.modifier_state.group) return;
	//c.modifier_state = new_state;
	//if (c.toplevel && c.toplevel->client->keyboard)
	//	wl_keyboard_send_modifiers (c.toplevel->client->keyboard, 0, c.modifier_state.depressed, c.modifier_state.latched, c.modifier_state.locked, c.modifier_state.group);
}
static void handle_terminate() {
	c.running = false;
}
static struct callbacks callbacks = {&handle_resize_event, &handle_draw_event, &handle_mouse_motion_event, &handle_mouse_button_event, &handle_key_event, &handle_modifiers_changed, &handle_terminate};

static void draw (void) {
	glClearColor (0, 0, 0, 0);
	glClear (GL_COLOR_BUFFER_BIT);
	
	if (c.renderer && c.toplevel && c.toplevel->texture)
		texture_draw(c.renderer, c.toplevel->texture, -1.0, -1.0, 1.0, 1.0);
		
	glFlush ();
	backend_swap_buffers ();
	c.redraw_needed = false;
}

int main(int argc, char *argv[]) {
	if (argc == 2 && !strcmp("-f", argv[1])) trace_funcs = true;
	LOGV("Starting lorie server");
	signal(SIGSEGV, handler);
	backend_init (&callbacks);
	//setenv("WAYLAND_DEBUG", "1", 1);
	memset(&c, 0, sizeof(c));
	wl_list_init (&c.clients);
	wl_list_init (&c.surfaces);
	wl_list_init (&c.shell_surfaces);
	c.width = 1024;
	c.height = 600;
	c.running = true;
	c.display = wl_display_create ();
	wl_display_add_socket_auto (c.display);

	wl_global_create (c.display, &wl_compositor_interface, 4, NULL, &wl_compositor_bind);
	wl_global_create (c.display, &wl_shell_interface, 1, NULL, &wl_shell_bind);
	wl_global_create (c.display, &wl_seat_interface, 4, NULL, &wl_seat_bind);
	wl_global_create (c.display, &wl_output_interface, 3, NULL, &wl_output_bind);
	wl_display_init_shm (c.display);
	
	c.renderer = renderer_create();
	
	struct wl_event_loop *event_loop = wl_display_get_event_loop (c.display);
	int wayland_fd = wl_event_loop_get_fd (event_loop);
	while (c.running) {
		wl_event_loop_dispatch (event_loop, 0);
		backend_dispatch_nonblocking ();
		wl_display_flush_clients (c.display);
		if (c.redraw_needed) {
			draw ();
		}
		//	usleep(50000);
		
		backend_wait_for_events (wayland_fd);
	}
	
	wl_display_destroy (c.display);
	return 0;
}
