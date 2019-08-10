#pragma once
#include <wayland-server.hpp>
class wl_callback_t : public wl_resource_t {
public:
	void init() override;
	void send_done(uint32_t callback_data);
};

class wl_compositor_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_create_surface(uint32_t id) = 0;
	virtual void request_create_region(uint32_t id) = 0;
};

class wl_buffer_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_destroy() = 0;
	void send_release();
};

class wl_data_offer_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_accept(uint32_t serial,
					 const char *mime_type) = 0;
	virtual void request_receive(const char *mime_type,
					  int32_t fd) = 0;
	virtual void request_destroy() = 0;
	virtual void request_finish() = 0;
	virtual void request_set_actions(uint32_t dnd_actions,
					      uint32_t preferred_action) = 0;
	void send_offer(const char *mime_type);
	void send_source_actions(uint32_t source_actions);
	void send_action(uint32_t dnd_action);
};

class wl_data_source_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_offer(const char *mime_type) = 0;
	virtual void request_destroy() = 0;
	virtual void request_set_actions(uint32_t dnd_actions) = 0;
	void send_target(const char *mime_type);
	void send_send(const char *mime_type, int32_t fd);
	void send_cancelled();
	void send_dnd_drop_performed();
	void send_dnd_finished();
	void send_action(uint32_t dnd_action);
};

class wl_data_device_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_start_drag(struct wl_resource *source,
					     struct wl_resource *origin,
					     struct wl_resource *icon,
					     uint32_t serial) = 0;
	virtual void request_set_selection(struct wl_resource *source,
						uint32_t serial) = 0;
	virtual void request_release() = 0;
	void send_data_offer(struct wl_resource *id);
	void send_enter(uint32_t serial, struct wl_resource *surface, wl_fixed_t x, wl_fixed_t y, struct wl_resource *id);
	void send_leave();
	void send_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y);
	void send_drop();
	void send_selection(struct wl_resource *id);
};

class wl_data_device_manager_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_create_data_source(uint32_t id) = 0;
	virtual void request_get_data_device(uint32_t id,
						  struct wl_resource *seat) = 0;
};

class wl_shell_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_get_shell_surface(uint32_t id,
						    struct wl_resource *surface) = 0;
};

class wl_shell_surface_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_pong(uint32_t serial) = 0;
	virtual void request_move(struct wl_resource *seat,
				       uint32_t serial) = 0;
	virtual void request_resize(struct wl_resource *seat,
					 uint32_t serial,
					 uint32_t edges) = 0;
	virtual void request_set_toplevel() = 0;
	virtual void request_set_transient(struct wl_resource *parent,
						int32_t x,
						int32_t y,
						uint32_t flags) = 0;
	virtual void request_set_fullscreen(uint32_t method,
						 uint32_t framerate,
						 struct wl_resource *output) = 0;
	virtual void request_set_popup(struct wl_resource *seat,
					    uint32_t serial,
					    struct wl_resource *parent,
					    int32_t x,
					    int32_t y,
					    uint32_t flags) = 0;
	virtual void request_set_maximized(struct wl_resource *output) = 0;
	virtual void request_set_title(const char *title) = 0;
	virtual void request_set_class(const char *class_) = 0;
	void send_ping(uint32_t serial);
	void send_configure(uint32_t edges, int32_t width, int32_t height);
	void send_popup_done();
};

class wl_surface_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_destroy() = 0;
	virtual void request_attach(struct wl_resource *buffer,
					 int32_t x,
					 int32_t y) = 0;
	virtual void request_damage(int32_t x,
					 int32_t y,
					 int32_t width,
					 int32_t height) = 0;
	virtual void request_frame(uint32_t callback) = 0;
	virtual void request_set_opaque_region(struct wl_resource *region) = 0;
	virtual void request_set_input_region(struct wl_resource *region) = 0;
	virtual void request_commit() = 0;
	virtual void request_set_buffer_transform(int32_t transform) = 0;
	virtual void request_set_buffer_scale(int32_t scale) = 0;
	virtual void request_damage_buffer(int32_t x,
						int32_t y,
						int32_t width,
						int32_t height) = 0;
	void send_enter(struct wl_resource *output);
	void send_leave(struct wl_resource *output);
};

class wl_seat_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_get_pointer(uint32_t id) = 0;
	virtual void request_get_keyboard(uint32_t id) = 0;
	virtual void request_get_touch(uint32_t id) = 0;
	virtual void request_release() = 0;
	void send_capabilities(uint32_t capabilities);
	void send_name(const char *name);
};

class wl_pointer_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_set_cursor(uint32_t serial,
					     struct wl_resource *surface,
					     int32_t hotspot_x,
					     int32_t hotspot_y) = 0;
	virtual void request_release() = 0;
	void send_enter(uint32_t serial, struct wl_resource *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
	void send_leave(uint32_t serial, struct wl_resource *surface);
	void send_motion(uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);
	void send_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
	void send_axis(uint32_t time, uint32_t axis, wl_fixed_t value);
	void send_frame();
	void send_axis_source(uint32_t axis_source);
	void send_axis_stop(uint32_t time, uint32_t axis);
	void send_axis_discrete(uint32_t axis, int32_t discrete);
};

class wl_keyboard_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_release() = 0;
	void send_keymap(uint32_t format, int32_t fd, uint32_t size);
	void send_enter(uint32_t serial, struct wl_resource *surface, struct wl_array *keys);
	void send_leave(uint32_t serial, struct wl_resource *surface);
	void send_key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
	void send_modifiers(uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
	void send_repeat_info(int32_t rate, int32_t delay);
};

class wl_touch_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_release() = 0;
	void send_down(uint32_t serial, uint32_t time, struct wl_resource *surface, int32_t id, wl_fixed_t x, wl_fixed_t y);
	void send_up(uint32_t serial, uint32_t time, int32_t id);
	void send_motion(uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y);
	void send_frame();
	void send_cancel();
	void send_shape(int32_t id, wl_fixed_t major, wl_fixed_t minor);
	void send_orientation(int32_t id, wl_fixed_t orientation);
};

class wl_output_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_release() = 0;
	void send_geometry(int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make, const char *model, int32_t transform);
	void send_mode(uint32_t flags, int32_t width, int32_t height, int32_t refresh);
	void send_done();
	void send_scale(int32_t factor);
};

class wl_region_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_destroy() = 0;
	virtual void request_add(int32_t x,
				      int32_t y,
				      int32_t width,
				      int32_t height) = 0;
	virtual void request_subtract(int32_t x,
					   int32_t y,
					   int32_t width,
					   int32_t height) = 0;
};

class wl_subcompositor_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_destroy() = 0;
	virtual void request_get_subsurface(uint32_t id,
						 struct wl_resource *surface,
						 struct wl_resource *parent) = 0;
};

class wl_subsurface_t : public wl_resource_t {
public:
	void init() override;
	virtual void request_destroy() = 0;
	virtual void request_set_position(int32_t x,
					       int32_t y) = 0;
	virtual void request_place_above(struct wl_resource *sibling) = 0;
	virtual void request_place_below(struct wl_resource *sibling) = 0;
	virtual void request_set_sync() = 0;
	virtual void request_set_desync() = 0;
};


struct wl_callback_interface;
struct wl_compositor_interface;
struct wl_buffer_interface;
struct wl_data_offer_interface;
struct wl_data_source_interface;
struct wl_data_device_interface;
struct wl_data_device_manager_interface;
struct wl_shell_interface;
struct wl_shell_surface_interface;
struct wl_surface_interface;
struct wl_seat_interface;
struct wl_pointer_interface;
struct wl_keyboard_interface;
struct wl_touch_interface;
struct wl_output_interface;
struct wl_region_interface;
struct wl_subcompositor_interface;
struct wl_subsurface_interface;

extern struct wl_compositor_interface wl_compositor_interface_implementation;
extern struct wl_buffer_interface wl_buffer_interface_implementation;
extern struct wl_data_offer_interface wl_data_offer_interface_implementation;
extern struct wl_data_source_interface wl_data_source_interface_implementation;
extern struct wl_data_device_interface wl_data_device_interface_implementation;
extern struct wl_data_device_manager_interface wl_data_device_manager_interface_implementation;
extern struct wl_shell_interface wl_shell_interface_implementation;
extern struct wl_shell_surface_interface wl_shell_surface_interface_implementation;
extern struct wl_surface_interface wl_surface_interface_implementation;
extern struct wl_seat_interface wl_seat_interface_implementation;
extern struct wl_pointer_interface wl_pointer_interface_implementation;
extern struct wl_keyboard_interface wl_keyboard_interface_implementation;
extern struct wl_touch_interface wl_touch_interface_implementation;
extern struct wl_output_interface wl_output_interface_implementation;
extern struct wl_region_interface wl_region_interface_implementation;
extern struct wl_subcompositor_interface wl_subcompositor_interface_implementation;
extern struct wl_subsurface_interface wl_subsurface_interface_implementation;

