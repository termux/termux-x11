#include <lorie_compositor.hpp>
#include <unistd.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

using namespace wayland;

LorieCompositor::LorieCompositor() :
display(dpy),
global_compositor(dpy),
global_seat(dpy),
global_output(dpy),
global_shell(dpy),
renderer(*this),
toplevel(renderer.toplevel_surface),
cursor(renderer.cursor_surface) {
    dpy.on_client = [=](client_t* client) {
		client->user_data() = new client_data;
        client->on_destroy = [=] {
            if (toplevel && toplevel->client() == client)
                set_toplevel(nullptr);

            if (cursor && cursor->client() == client)
                set_cursor(nullptr, 0, 0);

            LOGI("Client destroyed");
        };
    };
    global_compositor.on_bind = [=, this] (client_t*, compositor_t* compositor) {
        compositor->on_create_region = [](region_t* region) {
            region->on__destroy = [=] {
                region->destroy();
            };
        };
        compositor->on_create_surface = [=](surface_t* surface) {
            auto data = new surface_data;
            data->surface = surface;
            surface->user_data() = data;
            surface->on_attach = [=](buffer_t* b, int32_t, int32_t) {
                data->buffer = b;
                if (!b || !b->is_shm())
                    data->texture.set_data(&renderer, 0, 0, nullptr);
                else
                    data->texture.set_data(&renderer, b->shm_width(), b->shm_height(), b->shm_data());
            };
            surface->on_damage = [=](int32_t, int32_t, int32_t, int32_t) {
                data->texture.damage(0, 0, 0, 0);
            };
            surface->on_frame = [=] (callback_t* cb) {
                data->frame_callback = cb;
            };
            surface->on_commit = [=] {
                data->buffer->release();
                data->frame_callback->done(resource_t::timestamp());
            };
            surface->on__destroy = [=] {
                surface->destroy();
            };
        };
    };
    global_seat.on_bind = [=, this](client_t* client, seat_t* seat) {
        seat->capabilities (seat_capability(input_capabilities()));
        seat->name("default");

        auto data = any_cast<client_data*>(client->user_data());

        seat->on_get_pointer = [=](pointer_t* pointer) {
            LOGV("Client requested seat pointer");
            data->pointer = pointer;
            if (toplevel)
                pointer->enter(next_serial(), toplevel, 0, 0);

            pointer->on_set_cursor = [=](uint32_t, wayland::surface_t* sfc, int32_t x, int32_t y) {
                set_cursor(sfc, (uint32_t) x, (uint32_t) y);
            };
            pointer->on_release = [=] {
                pointer->destroy();
            };
        };
        seat->on_get_keyboard = [=](keyboard_t* kbd) {
        LOGV("Client requested seat keyboard");
            data->kbd = kbd;

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

            if (toplevel)
                kbd->enter(next_serial(), toplevel, &keys);

            kbd->on_release = [=] {
                kbd->destroy();
            };
        };
        seat->on_get_touch = [=](touch_t* touch) {
            data->touch = touch;
            touch->on_release = [=] {
                touch->destroy();
            };
        };
    };
    global_output.on_bind = [=, this](client_t* client, output_t* output) {
		auto data = any_cast<client_data*>(client->user_data());
        data->output = output;
        report_mode(output);

        output->on_release = [=]{
            output->destroy();
        };
    };
    global_shell.on_bind = [=](client_t* client, shell_t* shell) {
		auto data = any_cast<client_data*>(client->user_data());

        shell->on_get_shell_surface = [=] (shell_surface_t* shell, surface_t* sfc) {
            shell->on_set_toplevel = [=] () {
                set_toplevel(sfc);
                if (data->pointer)
                    data->pointer->enter(next_serial(),sfc, 0, 0);

                if(data->kbd)
                    data->kbd->enter(next_serial(), sfc, nullptr);

                shell->configure(shell_surface_resize::none, renderer.width, renderer.height);
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
	wl_display_add_socket_auto(display);

	wl_event_loop_add_fd(wl_display_get_event_loop(display), queue.get_fd(), WL_EVENT_READABLE, &proc, this);

	wl_display_init_shm (display);

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

void LorieCompositor::set_toplevel(surface_t* surface) {
	renderer.set_toplevel(surface);
}

void LorieCompositor::set_cursor(surface_t* surface, uint32_t hotspot_x, uint32_t hotspot_y) {
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

void LorieCompositor::report_mode(wayland::output_t* output) const {
	output->geometry(0, 0, renderer.physical_width, renderer.physical_height, output_subpixel::unknown, "Lorie", "none", output_transform::normal);
	output->scale(1.0);
	output->mode(output_mode::current | output_mode::preferred, renderer.width, renderer.height, 60000);
	output->done();
}

void LorieCompositor::touch_down(uint32_t id, uint32_t x, uint32_t y) {
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	data->touch->down(next_serial(), resource_t::timestamp(), toplevel, id, x, y);
	renderer.setCursorVisibility(false);
}

void LorieCompositor::touch_motion(uint32_t id, uint32_t x, uint32_t y) {
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	data->touch->motion(resource_t::timestamp(), id, x, y);
	renderer.setCursorVisibility(false);
}

void LorieCompositor::touch_up(uint32_t id) {
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	data->touch->up(next_serial(), resource_t::timestamp(), id);
	renderer.setCursorVisibility(false);
}

void LorieCompositor::touch_frame() {
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	data->touch->frame();
	renderer.setCursorVisibility(false);
}

void LorieCompositor::pointer_motion(uint32_t x, uint32_t y) {
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	data->pointer->motion(resource_t::timestamp(), x, y);
	data->pointer->frame();

	renderer.setCursorVisibility(true);
	renderer.cursorMove(x, y);
}

void LorieCompositor::pointer_scroll(uint32_t axis, float value) {
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

    data->pointer->axis_discrete(pointer_axis(axis), (value >= 0) ? 1 : -1);
    data->pointer->axis(resource_t::timestamp(), pointer_axis(axis), value);
    data->pointer->frame();
	renderer.setCursorVisibility(true);
}

void LorieCompositor::pointer_button(uint32_t button, uint32_t state) {
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	LOGI("pointer button: %d %d", button, state);
	data->pointer->button(next_serial(), resource_t::timestamp(), button, pointer_button_state(state));
	data->pointer->frame();
	renderer.setCursorVisibility(true);
}

void LorieCompositor::keyboard_key(uint32_t key, uint32_t state) {
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	data->kbd->key (next_serial(), resource_t::timestamp(), key, keyboard_key_state(state));
}

void LorieCompositor::keyboard_key_modifiers(uint8_t depressed, uint8_t latched, uint8_t locked, uint8_t group) {
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	if (key_modifiers.depressed == depressed && 
		key_modifiers.latched == latched && 
		key_modifiers.locked == locked &&
		key_modifiers.group == group) return;

	key_modifiers.depressed = depressed;
	key_modifiers.latched = latched;
	key_modifiers.locked = locked;
	key_modifiers.group = group;

	data->kbd->modifiers(next_serial(), depressed, latched, locked, group);
}

void LorieCompositor::keyboard_keymap_changed() {
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	int fd = -1, size = -1;
	get_keymap(&fd, &size);
	if (fd == -1 || size == -1) {
		LOGE("Error while getting keymap from backend");
		return;
	}

	data->kbd->keymap(keyboard_keymap_format::xkb_v1, fd, size);
	close (fd);
}

uint32_t LorieCompositor::next_serial() const {
	if (display == nullptr) return 0;

	return wl_display_next_serial(display);
}

#pragma clang diagnostic pop
