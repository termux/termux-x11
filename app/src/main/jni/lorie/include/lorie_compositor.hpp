#pragma once
#include <lorie_wayland_server.hpp>
#include <wayland-server-protocol.hpp>
#include <xdg-shell-server-protocol.hpp>
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
		wayland::output_t* output{};
		wayland::pointer_t* pointer{};
		wayland::keyboard_t* kbd{};
		wayland::touch_t* touch{};
	};

	struct surface_data {
		uint32_t x{}, y{};
		bool damaged{};
		wayland::buffer_t *buffer{};
		wayland::callback_t *frame_callback{};
	};

	lorie_renderer renderer;

	wayland::surface_t* toplevel{};
	wayland::surface_t* cursor{};

// backend features
	virtual void backend_init() = 0;
	virtual void swap_buffers() = 0;
	virtual void get_keymap(int *fd, int *size) = 0;
	virtual ~lorie_compositor() {};

//private:
	wayland::display_t dpy;
	wayland::global_compositor_t global_compositor{dpy};
	wayland::global_seat_t global_seat{dpy};
	wayland::global_output_t global_output{dpy};
	wayland::global_shell_t global_shell{dpy};
	wayland::global_xdg_wm_base_t global_xdg_wm_base{dpy};
	struct wl_display *display = nullptr;

	lorie_message_queue queue;
private:
	uint32_t next_serial() const;
};
