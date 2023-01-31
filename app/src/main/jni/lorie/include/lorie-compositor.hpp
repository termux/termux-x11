#pragma once
#include <wayland-server.h>
#include <lorie-client.hpp>
#include <lorie-renderer.hpp>
#include <lorie-message-queue.hpp>
#include <LorieImpls.hpp>

class LorieRenderer;
class LorieBackend;
class LorieSurface;
class LorieOutput;
class LorieSeat;

class LorieCompositor;
class LorieCompositor {
public:
	LorieCompositor();
// compositor features
	void start();
	void post(std::function<void()> f);
	struct wl_event_source* add_fd_listener(int fd, uint32_t mask, wl_event_loop_fd_func_t func, void *data);

	void set_toplevel(LorieSurface *surface);
	void set_cursor(LorieSurface *surface, uint32_t hotspot_x, uint32_t hotspot_y);

	void terminate();
	void output_redraw();
	void output_resize(uint32_t width, uint32_t height, uint32_t physical_width, uint32_t physical_height);

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


	LorieRenderer renderer;
	LorieSurface*& toplevel;
	LorieSurface*& cursor;

// backend features
	virtual void backend_init() = 0;
	virtual uint32_t input_capabilities() = 0;
	virtual void swap_buffers() = 0;
	virtual void get_default_proportions(int32_t* width, int32_t* height) = 0;
	virtual void get_keymap(int *fd, int *size) = 0;
	virtual ~LorieCompositor() {};

//private:
	wayland::display_t dpy;
	wayland::global_seat_t global_seat;
	struct wl_display *display = nullptr;


	LorieMessageQueue queue;
private:
	wl_client_created_listener_t<LorieClient, LorieCompositor&> client_created_listener;

	struct LorieClient* get_toplevel_client();
	uint32_t next_serial();

	struct {
		uint32_t depressed = 0, latched = 0, locked = 0, group = 0;
	} key_modifiers;
};

class LorieClipboard {
public:
	LorieClipboard(LorieDataSource &data_source);
	LorieDataSource &data_source;
	int read_fd;

	struct wl_array contents;
	struct wl_array mime_types;
	struct wl_listener selection_listener;
	struct wl_listener destroy_listener;
	struct wl_event_source *source;
};

#include <lorie-message-queue.hpp>
