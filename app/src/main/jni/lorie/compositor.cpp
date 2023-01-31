#include <lorie-compositor.hpp>
#include <lorie-client.hpp>
#include <unistd.h>

#include <LorieImpls.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#undef LOGV
#define LOGV(fmt ...)

using namespace wayland;

LorieCompositor::LorieCompositor() :
display(dpy),
global_seat(dpy),
renderer(*this),
toplevel(renderer.toplevel_surface),
cursor(renderer.cursor_surface),
client_created_listener(*this) {
    global_seat.on_bind = [=, this](client_t* cl, seat_t* seat) {
        seat->capabilities (seat_capability(input_capabilities()));
        seat->name("default");

        LorieClient* client = LorieClient::get(*cl);

        seat->on_get_pointer = [=](pointer_t* pointer) {
            LOGV("Client requested seat pointer");
            client->pointer = pointer;
            auto surface = toplevel ? reinterpret_cast<wayland::surface_t*>(wayland::resource_t::get(toplevel->resource)) : nullptr;
            pointer->enter(next_serial(), surface, 0, 0);
            pointer->on_set_cursor = [=](uint32_t, wayland::surface_t* sfc, int32_t x, int32_t y) {
                auto surface = sfc?LorieSurface::from_wl_resource<LorieSurface>(sfc->c_ptr()):nullptr;
                set_cursor(surface, (uint32_t) x, (uint32_t) y);
            };
            pointer->on_release = [=] {
                pointer->destroy();
            };
        };
        seat->on_get_keyboard = [=](keyboard_t* kbd) {
        LOGV("Client requested seat keyboard");
            client->kbd = kbd;
            auto surface = toplevel ? reinterpret_cast<wayland::surface_t*>(wayland::resource_t::get(toplevel->resource)) : nullptr;
            int fd = -1, size = -1;
            get_keymap(&fd, &size);
            if (fd == -1 || size == -1) {
                LOGE("Error while getting keymap from backend");
                return;
            }

            kbd->keymap(wayland::keyboard_keymap_format::xkb_v1, fd, size);
            close (fd);

            wl_array keys{};
            wl_array_init(&keys);
            kbd->enter(next_serial(), surface, &keys);

            kbd->on_release = [=] {
                kbd->destroy();
            };
        };
        seat->on_get_touch = [=](touch_t* touch) {
            client->touch = touch;
            touch->on_release = [=] {
                touch->destroy();
            };
        };
    };
}

int proc(int fd, uint32_t mask, void *data) {
	LorieCompositor *b = static_cast<LorieCompositor*>(data);
	if (b == nullptr) {LOGF("b == nullptr"); return 0;}

	b->queue.run();
	return 0;
};

void LorieCompositor::start() {
	LogInit();
	LOGV("Starting compositor");
	//display = wl_display_create();
	wl_display_add_socket_auto(display);

	wl_event_loop_add_fd(wl_display_get_event_loop(display), queue.get_fd(), WL_EVENT_READABLE, &proc, this);

	wl_display_add_client_created_listener(display, &client_created_listener);

	wl_display_init_shm (display);
	wl_resource_t::global_create<LorieCompositor_>(display, this);
	wl_resource_t::global_create<LorieOutput>(display, this);
	wl_resource_t::global_create<LorieShell>(display, this);

	backend_init();

	wl_display_run(display);
}

void LorieCompositor::post(std::function<void()> f) {
    queue.write(f);
}

struct wl_event_source* LorieCompositor::add_fd_listener(int fd, uint32_t mask, wl_event_loop_fd_func_t func, void *data) {
	LOGV("Adding fd %d to event loop", fd);
	struct wl_event_loop* loop = nullptr;
	if (display != nullptr)
		loop = wl_display_get_event_loop(display);

	if (loop != nullptr)
		return wl_event_loop_add_fd(loop, fd, mask, func, data);

	return nullptr;
}

void LorieCompositor::terminate() {
	LOGV("Terminating compositor");
	if (display != nullptr)
		wl_display_terminate(display);
}

void LorieCompositor::set_toplevel(LorieSurface *surface) {
	renderer.set_toplevel(surface);
}

void LorieCompositor::set_cursor(LorieSurface *surface, uint32_t hotspot_x, uint32_t hotspot_y) {
    renderer.set_cursor(surface, hotspot_x, hotspot_y);
}

void LorieCompositor::output_redraw() {
	LOGV("Requested redraw");
	renderer.requestRedraw();
}

void LorieCompositor::output_resize(uint32_t width, uint32_t height, uint32_t physical_width, uint32_t physical_height) {
	// Xwayland segfaults without that line
	if (width == 0 || height == 0 || physical_width == 0 || physical_height == 0) return;
	renderer.resize(width, height, physical_width, physical_height);
	post([this]() {
		output_redraw();
	});
}

void LorieCompositor::touch_down(uint32_t id, uint32_t x, uint32_t y) {
	LorieClient *client = get_toplevel_client();
	if (toplevel == nullptr || client == nullptr) return;

	client->touch->down(next_serial(), LorieUtils::timestamp(), reinterpret_cast<surface_t*>(resource_t::get(toplevel->resource)), id, x, y);
	renderer.setCursorVisibility(false);
}

void LorieCompositor::touch_motion(uint32_t id, uint32_t x, uint32_t y) {
	LorieClient *client = get_toplevel_client();
	if (toplevel == nullptr || client == nullptr) return;

	client->touch->motion(LorieUtils::timestamp(), id, x, y);
	renderer.setCursorVisibility(false);
}

void LorieCompositor::touch_up(uint32_t id) {
	LorieClient *client = get_toplevel_client();
	if (toplevel == nullptr || client == nullptr) return;

	client->touch->up(next_serial(), LorieUtils::timestamp(), id);
	renderer.setCursorVisibility(false);
}

void LorieCompositor::touch_frame() {
	LorieClient *client = get_toplevel_client();
	if (toplevel == nullptr || client == nullptr) return;

	client->touch->frame();
	renderer.setCursorVisibility(false);
}

void LorieCompositor::pointer_motion(uint32_t x, uint32_t y) {
	LorieClient *client = get_toplevel_client();
	if (client == nullptr) return;

	client->pointer->motion(LorieUtils::timestamp(), x, y);
	client->pointer->frame();

	renderer.setCursorVisibility(true);
	renderer.cursorMove(x, y);
}

void LorieCompositor::pointer_scroll(uint32_t axis, float value) {
    LorieClient *client = get_toplevel_client();
    if (client == nullptr) return;

    client->pointer->axis_discrete(pointer_axis(axis), (value >= 0) ? 1 : -1);
    client->pointer->axis(LorieUtils::timestamp(), pointer_axis(axis), value);
    client->pointer->frame();
	renderer.setCursorVisibility(true);
}

void LorieCompositor::pointer_button(uint32_t button, uint32_t state) {
	LorieClient *client = get_toplevel_client();
	if (client == nullptr) return;

	LOGI("pointer button: %d %d", button, state);
	client->pointer->button(next_serial(), LorieUtils::timestamp(), button, pointer_button_state(state));
	client->pointer->frame();
	renderer.setCursorVisibility(true);
}

void LorieCompositor::keyboard_key(uint32_t key, uint32_t state) {
	LorieClient *client = get_toplevel_client();
	if (client == nullptr) return;

	client->kbd->key (next_serial(), LorieUtils::timestamp(), key, keyboard_key_state(state));
}

void LorieCompositor::keyboard_key_modifiers(uint8_t depressed, uint8_t latched, uint8_t locked, uint8_t group) {
	LorieClient *client = get_toplevel_client();
	if (client == nullptr) return;

	if (key_modifiers.depressed == depressed && 
		key_modifiers.latched == latched && 
		key_modifiers.locked == locked &&
		key_modifiers.group == group) return;

	key_modifiers.depressed = depressed;
	key_modifiers.latched = latched;
	key_modifiers.locked = locked;
	key_modifiers.group = group;

	client->kbd->modifiers(next_serial(), depressed, latched, locked, group);
}

void LorieCompositor::keyboard_keymap_changed() {
	LorieClient *client = get_toplevel_client();
	if (client == nullptr) return;

	int fd = -1, size = -1;
	get_keymap(&fd, &size);
	if (fd == -1 || size == -1) {
		LOGE("Error while getting keymap from backend");
		return;
	}

	client->kbd->keymap(keyboard_keymap_format::xkb_v1, fd, size);
	close (fd);
}

LorieClient* LorieCompositor::get_toplevel_client() {
	if (toplevel != nullptr && toplevel->client != nullptr && *(toplevel->client) != nullptr)
		return *(toplevel->client);
	return nullptr;
}

uint32_t LorieCompositor::next_serial() {
	if (display == nullptr) return 0;

	return wl_display_next_serial(display);
}

#pragma clang diagnostic pop
