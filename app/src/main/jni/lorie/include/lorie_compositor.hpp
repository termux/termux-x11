#pragma once
#include <lorie_wayland_server.hpp>
#include <wayland-server-protocol.hpp>
#include <lorie_renderer.hpp>
#include <lorie_message_queue.hpp>
#include "log.h"

class lorie_compositor {
public:
	lorie_compositor();
// compositor features
	void start();
	void post(std::function<void()> f);

	void terminate();
	void output_resize(int width, int height, uint32_t physical_width, uint32_t physical_height);
	void report_mode(wayland::output_t* output) const;

	void touch_down(uint32_t id, uint32_t x, uint32_t y);
	void touch_motion(uint32_t id, uint32_t x, uint32_t y);
	void touch_up(uint32_t id);
	void touch_frame();
	void pointer_motion(uint32_t x, uint32_t y); // absolute values
	void pointer_scroll(uint32_t axis, float value);
	void pointer_button(uint32_t button, uint32_t state);
	void keyboard_key(uint32_t key, wayland::keyboard_key_state state);

	struct client_data {
		wayland::output_t* output = nullptr;
		wayland::pointer_t* pointer = nullptr;
		wayland::keyboard_t* kbd = nullptr;
		wayland::touch_t* touch = nullptr;
	};

	struct surface_data {
		uint32_t x = 0, y = 0;
		lorie_texture texture;
		wayland::buffer_t *buffer = NULL;
		wayland::callback_t *frame_callback = NULL;
	};

	lorie_renderer renderer;

	wayland::surface_t*& toplevel;
	wayland::surface_t*& cursor;

// backend features
	virtual void backend_init() = 0;
	virtual void swap_buffers() = 0;
	virtual void get_keymap(int *fd, int *size) = 0;
	virtual ~lorie_compositor() {};

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
};
