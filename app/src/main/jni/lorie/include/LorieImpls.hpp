#pragma once

#include <wayland.hpp>
#include "log.h"

class LorieClient;

#include <lorie-renderer.hpp>

class LorieUtils {
public:
	static uint32_t timestamp();
};

class LorieCompositor;
class LorieDataOffer;
class LorieDataSource;
class LorieDataDevice;
class LorieDataDeviceManager;
class LorieShell;
class LorieShellSurface;
class LorieSurface;
class LorieSeat;
class LoriePointer;
class LorieKeyboard;
class LorieTouch;
class LorieOutput;
class LorieRegion;

class LorieRegion: public wl_region_t {
public:
	void on_destroy() override {};
	void on_create() override {};
	void request_destroy() override {destroy();};
	void request_add(int32_t x, int32_t y,  int32_t width, int32_t height) override {};
	void request_subtract(int32_t x, int32_t y,  int32_t width, int32_t height) override {};
};

class LorieSurface: public wl_surface_t {
public:
	void on_destroy() override {};
	void on_create() override {};

	uint32_t x = 0, y = 0;
	LorieTexture texture;
	struct wl_resource *buffer = NULL;
	struct wl_resource *frame_callback = NULL;
	
	void request_destroy() override {destroy();};
	void request_attach(struct wl_resource *buffer, int32_t x, int32_t y) override;
	void request_damage(int32_t x, int32_t y, int32_t width, int32_t height) override;
	void request_frame(uint32_t callback) override;
	void request_set_opaque_region(struct wl_resource *region) override {};
	void request_set_input_region(struct wl_resource *region) override {};
	void request_commit() override;
	void request_set_buffer_transform(int32_t transform) override {};
	void request_set_buffer_scale(int32_t scale) override {};
	void request_damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height) override {};
};

class LorieCompositor_: public wl_compositor_t {
public:
	void request_create_region(uint32_t id) override {
		wl_resource_t::create<LorieRegion>(client, id);
	};
	void request_create_surface(uint32_t id) override {
		wl_resource_t::create<LorieSurface>(client, id);
	};
	void on_destroy() override {};
	void on_create() override {};
};

class LorieDataSource: public wl_data_source_t {
	void on_destroy() override {};
	void on_create() override {};
	void request_offer(const char*) override {};
	void request_destroy() override {destroy();};
	void request_set_actions(uint32_t) override {};
};

class LorieDataDevice: public wl_data_device_t {
public:
	void on_destroy() override {};
	void on_create() override {};
	void request_start_drag(struct wl_resource *source, struct wl_resource *origin, struct wl_resource *icon, uint32_t serial) override {};
	void request_set_selection(struct wl_resource *source, uint32_t serial) override {};
	void request_release() override {destroy();};
};

class LorieDataDeviceManager: public wl_data_device_manager_t {
public:
	void on_destroy() override {};
	void on_create() override {};
	static void bind(struct wl_client *client, void *data, uint32_t version, uint32_t id);
	static void global_create(struct wl_display *display, void *data) {
		wl_resource_t::global_create<LorieDataDeviceManager>(display, &bind, data);
	};
	
	void request_create_data_source(uint32_t id) override {
		wl_resource_t::create<LorieDataSource>(client, id);
	};
	void request_get_data_device(uint32_t id, wl_resource*) override {
		wl_resource_t::create<LorieDataDevice>(client, id);
	};
};

class LorieSurface;
class LorieShellSurface: public wl_shell_surface_t {
public:
	void on_destroy() override {};
	void on_create() override {};
	
	LorieSurface *surface = NULL;
	void request_pong(uint32_t serial) override {};
	void request_move(struct wl_resource *seat, uint32_t serial) override {};
	void request_resize(struct wl_resource *seat, uint32_t serial, uint32_t edges) override {};
	void request_set_toplevel() override;
	void request_set_transient(struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags) override {};
	void request_set_fullscreen(uint32_t method, uint32_t framerate, struct wl_resource *output) override {};
	void request_set_popup(struct wl_resource *seat, uint32_t serial, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags) override {};
	void request_set_maximized(struct wl_resource *output) override {};
	void request_set_title(const char *title) override {};
	void request_set_class(const char *class_) override {};
};

class LorieShell: public wl_shell_t {
public:
	void on_destroy() override {};
	void on_create() override {};
	void request_get_shell_surface(uint32_t id, struct wl_resource *surface) override {
		LorieShellSurface* res = new LorieShellSurface;
		
		res->create(client, id, true);
		res->surface = LorieSurface::from_wl_resource<LorieSurface>(surface);
	}
};

class LoriePointer: public wl_pointer_t {
public:
	void on_destroy() override {};
	void on_create() override;
	void request_set_cursor(uint32_t, struct wl_resource*, int32_t, int32_t) override;
	void request_release() override {destroy();};
	void send_enter();
};

class LorieKeyboard: public wl_keyboard_t {
public:
	void on_destroy() override {};
	void on_create() override;
	void request_release() override {destroy();};
	void send_enter();
	void keymap_changed();
};

class LorieTouch: public wl_touch_t {
public:
	void on_destroy() override {};
	void on_create() override {};
	void request_release() override {destroy();};
};

class LorieSeat: public wl_seat_t {
public:
	void on_destroy() override {};
	void on_create() override;
	
	void request_get_pointer(uint32_t id) override;
	void request_get_keyboard(uint32_t id) override;
	void request_get_touch(uint32_t id) override;
	void request_release() override {destroy();};
};

class LorieOutput: public wl_output_t {
public:
	void request_release() override {destroy();};
	void report_mode();
	void on_destroy() override;
	void on_create() override;
};
