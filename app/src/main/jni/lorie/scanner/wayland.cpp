#include "wayland.hpp"

void wl_callback_t::send_done(uint32_t callback_data) {
	 if (resource == nullptr) return;
	 wl_callback_send_done(resource, callback_data);
}

static void wl_compositor_request_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	wl_compositor_t* res = static_cast<wl_compositor_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_create_surface(id);
}
static void wl_compositor_request_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	wl_compositor_t* res = static_cast<wl_compositor_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_create_region(id);
}


struct wl_compositor_interface wl_compositor_interface_implementation = {
	wl_compositor_request_create_surface,
	wl_compositor_request_create_region
};

static void wl_buffer_request_destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_buffer_t* res = static_cast<wl_buffer_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_destroy();
}


struct wl_buffer_interface wl_buffer_interface_implementation = {
	wl_buffer_request_destroy
};

void wl_buffer_t::send_release() {
	 if (resource == nullptr) return;
	 wl_buffer_send_release(resource);
}

static void wl_data_offer_request_accept(struct wl_client *client, struct wl_resource *resource, uint32_t serial, const char *mime_type) {
	wl_data_offer_t* res = static_cast<wl_data_offer_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_accept(serial, mime_type);
}
static void wl_data_offer_request_receive(struct wl_client *client, struct wl_resource *resource, const char *mime_type, int32_t fd) {
	wl_data_offer_t* res = static_cast<wl_data_offer_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_receive(mime_type, fd);
}
static void wl_data_offer_request_destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_data_offer_t* res = static_cast<wl_data_offer_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_destroy();
}
static void wl_data_offer_request_finish(struct wl_client *client, struct wl_resource *resource) {
	wl_data_offer_t* res = static_cast<wl_data_offer_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_finish();
}
static void wl_data_offer_request_set_actions(struct wl_client *client, struct wl_resource *resource, uint32_t dnd_actions, uint32_t preferred_action) {
	wl_data_offer_t* res = static_cast<wl_data_offer_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_actions(dnd_actions, preferred_action);
}


struct wl_data_offer_interface wl_data_offer_interface_implementation = {
	wl_data_offer_request_accept,
	wl_data_offer_request_receive,
	wl_data_offer_request_destroy,
	wl_data_offer_request_finish,
	wl_data_offer_request_set_actions
};

void wl_data_offer_t::send_offer(const char *mime_type) {
	 if (resource == nullptr) return;
	 wl_data_offer_send_offer(resource, mime_type);
}
void wl_data_offer_t::send_source_actions(uint32_t source_actions) {
	 if (resource == nullptr) return;
	 wl_data_offer_send_source_actions(resource, source_actions);
}
void wl_data_offer_t::send_action(uint32_t dnd_action) {
	 if (resource == nullptr) return;
	 wl_data_offer_send_action(resource, dnd_action);
}

static void wl_data_source_request_offer(struct wl_client *client, struct wl_resource *resource, const char *mime_type) {
	wl_data_source_t* res = static_cast<wl_data_source_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_offer(mime_type);
}
static void wl_data_source_request_destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_data_source_t* res = static_cast<wl_data_source_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_destroy();
}
static void wl_data_source_request_set_actions(struct wl_client *client, struct wl_resource *resource, uint32_t dnd_actions) {
	wl_data_source_t* res = static_cast<wl_data_source_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_actions(dnd_actions);
}


struct wl_data_source_interface wl_data_source_interface_implementation = {
	wl_data_source_request_offer,
	wl_data_source_request_destroy,
	wl_data_source_request_set_actions
};

void wl_data_source_t::send_target(const char *mime_type) {
	 if (resource == nullptr) return;
	 wl_data_source_send_target(resource, mime_type);
}
void wl_data_source_t::send_send(const char *mime_type, int32_t fd) {
	 if (resource == nullptr) return;
	 wl_data_source_send_send(resource, mime_type, fd);
}
void wl_data_source_t::send_cancelled() {
	 if (resource == nullptr) return;
	 wl_data_source_send_cancelled(resource);
}
void wl_data_source_t::send_dnd_drop_performed() {
	 if (resource == nullptr) return;
	 wl_data_source_send_dnd_drop_performed(resource);
}
void wl_data_source_t::send_dnd_finished() {
	 if (resource == nullptr) return;
	 wl_data_source_send_dnd_finished(resource);
}
void wl_data_source_t::send_action(uint32_t dnd_action) {
	 if (resource == nullptr) return;
	 wl_data_source_send_action(resource, dnd_action);
}

static void wl_data_device_request_start_drag(struct wl_client *client, struct wl_resource *resource, struct wl_resource *source, struct wl_resource *origin, struct wl_resource *icon, uint32_t serial) {
	wl_data_device_t* res = static_cast<wl_data_device_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_start_drag(source, origin, icon, serial);
}
static void wl_data_device_request_set_selection(struct wl_client *client, struct wl_resource *resource, struct wl_resource *source, uint32_t serial) {
	wl_data_device_t* res = static_cast<wl_data_device_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_selection(source, serial);
}
static void wl_data_device_request_release(struct wl_client *client, struct wl_resource *resource) {
	wl_data_device_t* res = static_cast<wl_data_device_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_release();
}


struct wl_data_device_interface wl_data_device_interface_implementation = {
	wl_data_device_request_start_drag,
	wl_data_device_request_set_selection,
	wl_data_device_request_release
};

void wl_data_device_t::send_data_offer(struct wl_resource *id) {
	 if (resource == nullptr) return;
	 wl_data_device_send_data_offer(resource, id);
}
void wl_data_device_t::send_enter(uint32_t serial, struct wl_resource *surface, wl_fixed_t x, wl_fixed_t y, struct wl_resource *id) {
	 if (resource == nullptr) return;
	 wl_data_device_send_enter(resource, serial, surface, x, y, id);
}
void wl_data_device_t::send_leave() {
	 if (resource == nullptr) return;
	 wl_data_device_send_leave(resource);
}
void wl_data_device_t::send_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y) {
	 if (resource == nullptr) return;
	 wl_data_device_send_motion(resource, time, x, y);
}
void wl_data_device_t::send_drop() {
	 if (resource == nullptr) return;
	 wl_data_device_send_drop(resource);
}
void wl_data_device_t::send_selection(struct wl_resource *id) {
	 if (resource == nullptr) return;
	 wl_data_device_send_selection(resource, id);
}

static void wl_data_device_manager_request_create_data_source(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	wl_data_device_manager_t* res = static_cast<wl_data_device_manager_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_create_data_source(id);
}
static void wl_data_device_manager_request_get_data_device(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *seat) {
	wl_data_device_manager_t* res = static_cast<wl_data_device_manager_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_get_data_device(id, seat);
}


struct wl_data_device_manager_interface wl_data_device_manager_interface_implementation = {
	wl_data_device_manager_request_create_data_source,
	wl_data_device_manager_request_get_data_device
};

static void wl_shell_request_get_shell_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface) {
	wl_shell_t* res = static_cast<wl_shell_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_get_shell_surface(id, surface);
}


struct wl_shell_interface wl_shell_interface_implementation = {
	wl_shell_request_get_shell_surface
};

static void wl_shell_surface_request_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial) {
	wl_shell_surface_t* res = static_cast<wl_shell_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_pong(serial);
}
static void wl_shell_surface_request_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial) {
	wl_shell_surface_t* res = static_cast<wl_shell_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_move(seat, serial);
}
static void wl_shell_surface_request_resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, uint32_t edges) {
	wl_shell_surface_t* res = static_cast<wl_shell_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_resize(seat, serial, edges);
}
static void wl_shell_surface_request_set_toplevel(struct wl_client *client, struct wl_resource *resource) {
	wl_shell_surface_t* res = static_cast<wl_shell_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_toplevel();
}
static void wl_shell_surface_request_set_transient(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags) {
	wl_shell_surface_t* res = static_cast<wl_shell_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_transient(parent, x, y, flags);
}
static void wl_shell_surface_request_set_fullscreen(struct wl_client *client, struct wl_resource *resource, uint32_t method, uint32_t framerate, struct wl_resource *output) {
	wl_shell_surface_t* res = static_cast<wl_shell_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_fullscreen(method, framerate, output);
}
static void wl_shell_surface_request_set_popup(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags) {
	wl_shell_surface_t* res = static_cast<wl_shell_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_popup(seat, serial, parent, x, y, flags);
}
static void wl_shell_surface_request_set_maximized(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output) {
	wl_shell_surface_t* res = static_cast<wl_shell_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_maximized(output);
}
static void wl_shell_surface_request_set_title(struct wl_client *client, struct wl_resource *resource, const char *title) {
	wl_shell_surface_t* res = static_cast<wl_shell_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_title(title);
}
static void wl_shell_surface_request_set_class(struct wl_client *client, struct wl_resource *resource, const char *class_) {
	wl_shell_surface_t* res = static_cast<wl_shell_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_class(class_);
}


struct wl_shell_surface_interface wl_shell_surface_interface_implementation = {
	wl_shell_surface_request_pong,
	wl_shell_surface_request_move,
	wl_shell_surface_request_resize,
	wl_shell_surface_request_set_toplevel,
	wl_shell_surface_request_set_transient,
	wl_shell_surface_request_set_fullscreen,
	wl_shell_surface_request_set_popup,
	wl_shell_surface_request_set_maximized,
	wl_shell_surface_request_set_title,
	wl_shell_surface_request_set_class
};

void wl_shell_surface_t::send_ping(uint32_t serial) {
	 if (resource == nullptr) return;
	 wl_shell_surface_send_ping(resource, serial);
}
void wl_shell_surface_t::send_configure(uint32_t edges, int32_t width, int32_t height) {
	 if (resource == nullptr) return;
	 wl_shell_surface_send_configure(resource, edges, width, height);
}
void wl_shell_surface_t::send_popup_done() {
	 if (resource == nullptr) return;
	 wl_shell_surface_send_popup_done(resource);
}

static void wl_surface_request_destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_surface_t* res = static_cast<wl_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_destroy();
}
static void wl_surface_request_attach(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer, int32_t x, int32_t y) {
	wl_surface_t* res = static_cast<wl_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_attach(buffer, x, y);
}
static void wl_surface_request_damage(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	wl_surface_t* res = static_cast<wl_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_damage(x, y, width, height);
}
static void wl_surface_request_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback) {
	wl_surface_t* res = static_cast<wl_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_frame(callback);
}
static void wl_surface_request_set_opaque_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {
	wl_surface_t* res = static_cast<wl_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_opaque_region(region);
}
static void wl_surface_request_set_input_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {
	wl_surface_t* res = static_cast<wl_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_input_region(region);
}
static void wl_surface_request_commit(struct wl_client *client, struct wl_resource *resource) {
	wl_surface_t* res = static_cast<wl_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_commit();
}
static void wl_surface_request_set_buffer_transform(struct wl_client *client, struct wl_resource *resource, int32_t transform) {
	wl_surface_t* res = static_cast<wl_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_buffer_transform(transform);
}
static void wl_surface_request_set_buffer_scale(struct wl_client *client, struct wl_resource *resource, int32_t scale) {
	wl_surface_t* res = static_cast<wl_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_buffer_scale(scale);
}
static void wl_surface_request_damage_buffer(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	wl_surface_t* res = static_cast<wl_surface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_damage_buffer(x, y, width, height);
}


struct wl_surface_interface wl_surface_interface_implementation = {
	wl_surface_request_destroy,
	wl_surface_request_attach,
	wl_surface_request_damage,
	wl_surface_request_frame,
	wl_surface_request_set_opaque_region,
	wl_surface_request_set_input_region,
	wl_surface_request_commit,
	wl_surface_request_set_buffer_transform,
	wl_surface_request_set_buffer_scale,
	wl_surface_request_damage_buffer
};

void wl_surface_t::send_enter(struct wl_resource *output) {
	 if (resource == nullptr) return;
	 wl_surface_send_enter(resource, output);
}
void wl_surface_t::send_leave(struct wl_resource *output) {
	 if (resource == nullptr) return;
	 wl_surface_send_leave(resource, output);
}

static void wl_seat_request_get_pointer(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	wl_seat_t* res = static_cast<wl_seat_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_get_pointer(id);
}
static void wl_seat_request_get_keyboard(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	wl_seat_t* res = static_cast<wl_seat_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_get_keyboard(id);
}
static void wl_seat_request_get_touch(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	wl_seat_t* res = static_cast<wl_seat_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_get_touch(id);
}
static void wl_seat_request_release(struct wl_client *client, struct wl_resource *resource) {
	wl_seat_t* res = static_cast<wl_seat_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_release();
}


struct wl_seat_interface wl_seat_interface_implementation = {
	wl_seat_request_get_pointer,
	wl_seat_request_get_keyboard,
	wl_seat_request_get_touch,
	wl_seat_request_release
};

void wl_seat_t::send_capabilities(uint32_t capabilities) {
	 if (resource == nullptr) return;
	 wl_seat_send_capabilities(resource, capabilities);
}
void wl_seat_t::send_name(const char *name) {
	 if (resource == nullptr) return;
	 wl_seat_send_name(resource, name);
}

static void wl_pointer_request_set_cursor(struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *surface, int32_t hotspot_x, int32_t hotspot_y) {
	wl_pointer_t* res = static_cast<wl_pointer_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_cursor(serial, surface, hotspot_x, hotspot_y);
}
static void wl_pointer_request_release(struct wl_client *client, struct wl_resource *resource) {
	wl_pointer_t* res = static_cast<wl_pointer_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_release();
}


struct wl_pointer_interface wl_pointer_interface_implementation = {
	wl_pointer_request_set_cursor,
	wl_pointer_request_release
};

void wl_pointer_t::send_enter(uint32_t serial, struct wl_resource *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	 if (resource == nullptr) return;
	 wl_pointer_send_enter(resource, serial, surface, surface_x, surface_y);
}
void wl_pointer_t::send_leave(uint32_t serial, struct wl_resource *surface) {
	 if (resource == nullptr) return;
	 wl_pointer_send_leave(resource, serial, surface);
}
void wl_pointer_t::send_motion(uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	 if (resource == nullptr) return;
	 wl_pointer_send_motion(resource, time, surface_x, surface_y);
}
void wl_pointer_t::send_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
	 if (resource == nullptr) return;
	 wl_pointer_send_button(resource, serial, time, button, state);
}
void wl_pointer_t::send_axis(uint32_t time, uint32_t axis, wl_fixed_t value) {
	 if (resource == nullptr) return;
	 wl_pointer_send_axis(resource, time, axis, value);
}
void wl_pointer_t::send_frame() {
	 if (resource == nullptr) return;
	 wl_pointer_send_frame(resource);
}
void wl_pointer_t::send_axis_source(uint32_t axis_source) {
	 if (resource == nullptr) return;
	 wl_pointer_send_axis_source(resource, axis_source);
}
void wl_pointer_t::send_axis_stop(uint32_t time, uint32_t axis) {
	 if (resource == nullptr) return;
	 wl_pointer_send_axis_stop(resource, time, axis);
}
void wl_pointer_t::send_axis_discrete(uint32_t axis, int32_t discrete) {
	 if (resource == nullptr) return;
	 wl_pointer_send_axis_discrete(resource, axis, discrete);
}

static void wl_keyboard_request_release(struct wl_client *client, struct wl_resource *resource) {
	wl_keyboard_t* res = static_cast<wl_keyboard_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_release();
}


struct wl_keyboard_interface wl_keyboard_interface_implementation = {
	wl_keyboard_request_release
};

void wl_keyboard_t::send_keymap(uint32_t format, int32_t fd, uint32_t size) {
	 if (resource == nullptr) return;
	 wl_keyboard_send_keymap(resource, format, fd, size);
}
void wl_keyboard_t::send_enter(uint32_t serial, struct wl_resource *surface, struct wl_array *keys) {
	 if (resource == nullptr) return;
	 wl_keyboard_send_enter(resource, serial, surface, keys);
}
void wl_keyboard_t::send_leave(uint32_t serial, struct wl_resource *surface) {
	 if (resource == nullptr) return;
	 wl_keyboard_send_leave(resource, serial, surface);
}
void wl_keyboard_t::send_key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
	 if (resource == nullptr) return;
	 wl_keyboard_send_key(resource, serial, time, key, state);
}
void wl_keyboard_t::send_modifiers(uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
	 if (resource == nullptr) return;
	 wl_keyboard_send_modifiers(resource, serial, mods_depressed, mods_latched, mods_locked, group);
}
void wl_keyboard_t::send_repeat_info(int32_t rate, int32_t delay) {
	 if (resource == nullptr) return;
	 wl_keyboard_send_repeat_info(resource, rate, delay);
}

static void wl_touch_request_release(struct wl_client *client, struct wl_resource *resource) {
	wl_touch_t* res = static_cast<wl_touch_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_release();
}


struct wl_touch_interface wl_touch_interface_implementation = {
	wl_touch_request_release
};

void wl_touch_t::send_down(uint32_t serial, uint32_t time, struct wl_resource *surface, int32_t id, wl_fixed_t x, wl_fixed_t y) {
	 if (resource == nullptr) return;
	 wl_touch_send_down(resource, serial, time, surface, id, x, y);
}
void wl_touch_t::send_up(uint32_t serial, uint32_t time, int32_t id) {
	 if (resource == nullptr) return;
	 wl_touch_send_up(resource, serial, time, id);
}
void wl_touch_t::send_motion(uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y) {
	 if (resource == nullptr) return;
	 wl_touch_send_motion(resource, time, id, x, y);
}
void wl_touch_t::send_frame() {
	 if (resource == nullptr) return;
	 wl_touch_send_frame(resource);
}
void wl_touch_t::send_cancel() {
	 if (resource == nullptr) return;
	 wl_touch_send_cancel(resource);
}
void wl_touch_t::send_shape(int32_t id, wl_fixed_t major, wl_fixed_t minor) {
	 if (resource == nullptr) return;
	 wl_touch_send_shape(resource, id, major, minor);
}
void wl_touch_t::send_orientation(int32_t id, wl_fixed_t orientation) {
	 if (resource == nullptr) return;
	 wl_touch_send_orientation(resource, id, orientation);
}

static void wl_output_request_release(struct wl_client *client, struct wl_resource *resource) {
	wl_output_t* res = static_cast<wl_output_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_release();
}


struct wl_output_interface wl_output_interface_implementation = {
	wl_output_request_release
};

void wl_output_t::send_geometry(int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make, const char *model, int32_t transform) {
	 if (resource == nullptr) return;
	 wl_output_send_geometry(resource, x, y, physical_width, physical_height, subpixel, make, model, transform);
}
void wl_output_t::send_mode(uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
	 if (resource == nullptr) return;
	 wl_output_send_mode(resource, flags, width, height, refresh);
}
void wl_output_t::send_done() {
	 if (resource == nullptr) return;
	 wl_output_send_done(resource);
}
void wl_output_t::send_scale(int32_t factor) {
	 if (resource == nullptr) return;
	 wl_output_send_scale(resource, factor);
}

static void wl_region_request_destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_region_t* res = static_cast<wl_region_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_destroy();
}
static void wl_region_request_add(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	wl_region_t* res = static_cast<wl_region_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_add(x, y, width, height);
}
static void wl_region_request_subtract(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	wl_region_t* res = static_cast<wl_region_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_subtract(x, y, width, height);
}


struct wl_region_interface wl_region_interface_implementation = {
	wl_region_request_destroy,
	wl_region_request_add,
	wl_region_request_subtract
};

static void wl_subcompositor_request_destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_subcompositor_t* res = static_cast<wl_subcompositor_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_destroy();
}
static void wl_subcompositor_request_get_subsurface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface, struct wl_resource *parent) {
	wl_subcompositor_t* res = static_cast<wl_subcompositor_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_get_subsurface(id, surface, parent);
}


struct wl_subcompositor_interface wl_subcompositor_interface_implementation = {
	wl_subcompositor_request_destroy,
	wl_subcompositor_request_get_subsurface
};

static void wl_subsurface_request_destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_subsurface_t* res = static_cast<wl_subsurface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_destroy();
}
static void wl_subsurface_request_set_position(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y) {
	wl_subsurface_t* res = static_cast<wl_subsurface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_position(x, y);
}
static void wl_subsurface_request_place_above(struct wl_client *client, struct wl_resource *resource, struct wl_resource *sibling) {
	wl_subsurface_t* res = static_cast<wl_subsurface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_place_above(sibling);
}
static void wl_subsurface_request_place_below(struct wl_client *client, struct wl_resource *resource, struct wl_resource *sibling) {
	wl_subsurface_t* res = static_cast<wl_subsurface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_place_below(sibling);
}
static void wl_subsurface_request_set_sync(struct wl_client *client, struct wl_resource *resource) {
	wl_subsurface_t* res = static_cast<wl_subsurface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_sync();
}
static void wl_subsurface_request_set_desync(struct wl_client *client, struct wl_resource *resource) {
	wl_subsurface_t* res = static_cast<wl_subsurface_t*>(wl_resource_get_user_data(resource));
	if (res == nullptr) return;

	res->request_set_desync();
}


struct wl_subsurface_interface wl_subsurface_interface_implementation = {
	wl_subsurface_request_destroy,
	wl_subsurface_request_set_position,
	wl_subsurface_request_place_above,
	wl_subsurface_request_place_below,
	wl_subsurface_request_set_sync,
	wl_subsurface_request_set_desync
};
void wl_callback_t::init() {
	interface = &::wl_callback_interface;
	version = 1;
	implementation = nullptr;
}

void wl_compositor_t::init() {
	interface = &::wl_compositor_interface;
	version = 4;
	implementation = &wl_compositor_interface_implementation;
}

void wl_buffer_t::init() {
	interface = &::wl_buffer_interface;
	version = 1;
	implementation = &wl_buffer_interface_implementation;
}

void wl_data_offer_t::init() {
	interface = &::wl_data_offer_interface;
	version = 3;
	implementation = &wl_data_offer_interface_implementation;
}

void wl_data_source_t::init() {
	interface = &::wl_data_source_interface;
	version = 3;
	implementation = &wl_data_source_interface_implementation;
}

void wl_data_device_t::init() {
	interface = &::wl_data_device_interface;
	version = 3;
	implementation = &wl_data_device_interface_implementation;
}

void wl_data_device_manager_t::init() {
	interface = &::wl_data_device_manager_interface;
	version = 3;
	implementation = &wl_data_device_manager_interface_implementation;
}

void wl_shell_t::init() {
	interface = &::wl_shell_interface;
	version = 1;
	implementation = &wl_shell_interface_implementation;
}

void wl_shell_surface_t::init() {
	interface = &::wl_shell_surface_interface;
	version = 1;
	implementation = &wl_shell_surface_interface_implementation;
}

void wl_surface_t::init() {
	interface = &::wl_surface_interface;
	version = 4;
	implementation = &wl_surface_interface_implementation;
}

void wl_seat_t::init() {
	interface = &::wl_seat_interface;
	version = 4;
	implementation = &wl_seat_interface_implementation;
}

void wl_pointer_t::init() {
	interface = &::wl_pointer_interface;
	version = 7;
	implementation = &wl_pointer_interface_implementation;
}

void wl_keyboard_t::init() {
	interface = &::wl_keyboard_interface;
	version = 7;
	implementation = &wl_keyboard_interface_implementation;
}

void wl_touch_t::init() {
	interface = &::wl_touch_interface;
	version = 7;
	implementation = &wl_touch_interface_implementation;
}

void wl_output_t::init() {
	interface = &::wl_output_interface;
	version = 3;
	implementation = &wl_output_interface_implementation;
}

void wl_region_t::init() {
	interface = &::wl_region_interface;
	version = 1;
	implementation = &wl_region_interface_implementation;
}

void wl_subcompositor_t::init() {
	interface = &::wl_subcompositor_interface;
	version = 1;
	implementation = &wl_subcompositor_interface_implementation;
}

void wl_subsurface_t::init() {
	interface = &::wl_subsurface_interface;
	version = 1;
	implementation = &wl_subsurface_interface_implementation;
}

