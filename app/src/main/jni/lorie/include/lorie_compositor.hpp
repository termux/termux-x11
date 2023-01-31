#pragma once
#include <lorie_wayland_server.hpp>
#include <wayland-server-protocol.hpp>
#include <lorie_renderer.hpp>
#include <lorie_message_queue.hpp>
#include "log.h"

class LorieCompositor {
public:
	LorieCompositor();
// compositor features
	void start();
	void post(std::function<void()> f);
	struct wl_event_source* add_fd_listener(int fd, uint32_t mask, wl_event_loop_fd_func_t func, void *data);

	void set_toplevel(wayland::surface_t* surface);
	void set_cursor(wayland::surface_t* surface, uint32_t hotspot_x, uint32_t hotspot_y);

	void terminate();
	void output_redraw();
	void output_resize(uint32_t width, uint32_t height, uint32_t physical_width, uint32_t physical_height);
	void report_mode(wayland::output_t* output) const;

	void touch_down(uint32_t id, uint32_t x, uint32_t y);
	void touch_motion(uint32_t id, uint32_t x, uint32_t y);
	void touch_up(uint32_t id);
	void touch_frame();
	void pointer_motion(uint32_t x, uint32_t y); // absolute values
	void pointer_scroll(uint32_t axis, float value);
	void pointer_button(uint32_t button, uint32_t state);
	void keyboard_key(uint32_t key, uint32_t state);
	void keyboard_key_modifiers(uint8_t depressed, uint8_t latched, uint8_t locked, uint8_t group);
	void keyboard_keymap_changed();

	struct client_data {
		wayland::output_t* output = nullptr;
		wayland::pointer_t* pointer = nullptr;
		wayland::keyboard_t* kbd = nullptr;
		wayland::touch_t* touch = nullptr;
	};

	struct surface_data {
		wayland::surface_t* surface;
		uint32_t x = 0, y = 0;
		LorieTexture texture;
		wayland::buffer_t *buffer = NULL;
		wayland::callback_t *frame_callback = NULL;
	};

	LorieRenderer renderer;

	wayland::surface_t*& toplevel;
	wayland::surface_t*& cursor;

// backend features
	virtual void backend_init() = 0;
	virtual uint32_t input_capabilities() = 0;
	virtual void swap_buffers() = 0;
	virtual void get_default_proportions(int32_t* width, int32_t* height) = 0;
	virtual void get_keymap(int *fd, int *size) = 0;
	virtual ~LorieCompositor() {};

//private:
	wayland::display_t dpy;
	wayland::global_compositor_t global_compositor;
	wayland::global_seat_t global_seat;
	wayland::global_output_t global_output;
	wayland::global_shell_t global_shell;
	struct wl_display *display = nullptr;

	lorie_message_queue queue;
private:
	uint32_t next_serial() const;

	struct {
		uint32_t depressed = 0, latched = 0, locked = 0, group = 0;
	} key_modifiers;
};
