#include <wayland-server.h>

static void no_op_func(void) {}
#define NO_OP(type) ((type) &no_op_func)

// wl_region
typedef void (*wl_region_destroy_t)(struct wl_client *client, struct wl_resource *resource);
typedef void (*wl_region_add_t)(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
typedef void (*wl_region_subtract_t) (struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);

// wl_shell_surface
typedef void (*wl_shell_surface_pong_t)(struct wl_client *client, struct wl_resource *resource, uint32_t serial);
typedef void (*wl_shell_surface_move_t)(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial);
typedef void (*wl_shell_surface_resize_t)(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, uint32_t edges);	       
typedef void (*wl_shell_surface_set_toplevel_t)(struct wl_client *client, struct wl_resource *resource);
typedef void (*wl_shell_surface_set_transient_t)(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags);
typedef void (*wl_shell_surface_set_fullscreen_t)(struct wl_client *client, struct wl_resource *resource, uint32_t method, uint32_t framerate, struct wl_resource *output);
typedef void (*wl_shell_surface_set_popup_t)(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags);
typedef void (*wl_shell_surface_set_maximized_t)(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output);
typedef void (*wl_shell_surface_set_title_t)(struct wl_client *client, struct wl_resource *resource, const char *title);
typedef void (*wl_shell_surface_set_class_t)(struct wl_client *client, struct wl_resource *resource, const char *class_);

// wl_surface
typedef void (*wl_surface_destroy_t)(struct wl_client *client, struct wl_resource *resource);
typedef void (*wl_surface_attach_t)(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer, int32_t x, int32_t y);
typedef void (*wl_surface_damage_t)(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
typedef void (*wl_surface_frame_t)(struct wl_client *client, struct wl_resource *resource, uint32_t callback);
typedef void (*wl_surface_set_opaque_region_t)(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region);
typedef void (*wl_surface_set_input_region_t)(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region);
typedef void (*wl_surface_commit_t)(struct wl_client *client, struct wl_resource *resource);
typedef void (*wl_surface_set_buffer_transform_t)(struct wl_client *client, struct wl_resource *resource, int32_t transform);
typedef void (*wl_surface_set_buffer_scale_t)(struct wl_client *client, struct wl_resource *resource, int32_t scale);

// wl_seat, wl_keyboard, wl_pointer
typedef void (*wl_seat_get_pointer_t)(struct wl_client *client, struct wl_resource *resource, uint32_t id);
typedef void (*wl_seat_get_keyboard_t)(struct wl_client *client, struct wl_resource *resource, uint32_t id);
typedef void (*wl_seat_get_touch_t)(struct wl_client *client, struct wl_resource *resource, uint32_t id);
typedef void (*wl_pointer_set_cursor_t)(struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *_surface, int32_t hotspot_x, int32_t hotspot_y);
typedef void (*wl_pointer_release_t)(struct wl_client *client, struct wl_resource *resource);
typedef void (*wl_keyboard_release_t)(struct wl_client *client, struct wl_resource *resource);

// No op interfaces
static struct wl_region_interface lorie_region_interface = {NO_OP(wl_region_destroy_t), NO_OP(wl_region_add_t), NO_OP(wl_region_subtract_t)};
static struct wl_pointer_interface lorie_pointer_interface = {NO_OP(wl_pointer_set_cursor_t), NO_OP(wl_pointer_release_t)};
static struct wl_keyboard_interface lorie_keyboard_interface = {NO_OP(wl_keyboard_release_t)};

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
