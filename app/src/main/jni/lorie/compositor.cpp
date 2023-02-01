#include <lorie_compositor.hpp>
#include <unistd.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

using namespace wayland;

lorie_compositor::lorie_compositor() :
display(dpy),
renderer(*this) {
    dpy.on_client = [=](client_t* client) {
		client->user_data() = new client_data;
        client->on_destroy = [=] {
            if (toplevel && toplevel->client() == client)
                renderer.set_toplevel(nullptr);

            if (cursor && cursor->client() == client)
                renderer.set_cursor(nullptr, 0, 0);

            LOGI("Client destroyed");
        };
    };
    global_compositor.on_bind = [=] (client_t*, compositor_t* compositor) {
        compositor->on_create_region = [](region_t* region) {
            region->on__destroy = [=] {
                region->destroy();
            };
        };
        compositor->on_create_surface = [=](surface_t* surface) {
            auto data = new surface_data;
            surface->user_data() = data;
            surface->on_attach = [=](buffer_t* b, int32_t, int32_t) {
                if (data->buffer)
                    data->buffer->release();
                data->buffer = b;
            };
            surface->on_damage = [=](int32_t, int32_t, int32_t, int32_t) {
                data->damaged = true;
            };
            surface->on_frame = [=] (callback_t* cb) {
                data->frame_callback = cb;
            };
            surface->on_commit = [=] {
                renderer.request_redraw();
                data->frame_callback->done(resource_t::timestamp());
            };
            surface->on__destroy = [=] {
                surface->destroy();
            };
        };
    };
    global_seat.on_bind = [=, this](client_t* client, seat_t* seat) {
        seat->capabilities (seat_capability::touch | seat_capability::keyboard | seat_capability::pointer);
        seat->name("default");

        auto data = any_cast<client_data*>(client->user_data());

        seat->on_get_pointer = [=](pointer_t* pointer) {
            LOGV("Client requested seat pointer");
            data->pointer = pointer;
            if (toplevel)
                pointer->enter(next_serial(), toplevel, 0, 0);

            pointer->on_set_cursor = [=](uint32_t, wayland::surface_t* sfc, int32_t x, int32_t y) {
                renderer.set_cursor(sfc, (uint32_t) x, (uint32_t) y);
                if (sfc)
                    any_cast<surface_data*>(sfc->user_data())->damaged = true;
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
                wl_array keys{};
                wl_array_init(&keys);
                renderer.set_toplevel(sfc);
                if (data->pointer)
                    data->pointer->enter(next_serial(),sfc, 0, 0);

                if(data->kbd)
                    data->kbd->enter(next_serial(), sfc, &keys);

                shell->configure(shell_surface_resize::none, renderer.width, renderer.height);
            };
        };
    };
    global_xdg_wm_base.on_bind = [=, this](client_t* client, xdg_wm_base_t* wm_base) {
        wm_base->on_get_xdg_surface = [=, this](xdg_surface_t* xdg_surface, surface_t* sfc) {
            xdg_surface->on_get_toplevel = [=, this](xdg_toplevel_t*) {
                auto data = any_cast<client_data*>(toplevel->client()->user_data());
                wl_array keys{};
                wl_array_init(&keys);
                renderer.set_toplevel(sfc);
                if (data->pointer)
                    data->pointer->enter(next_serial(),sfc, 0, 0);
                if (data->kbd)
                    data->kbd->enter(next_serial(), sfc, &keys);
            };
        };
        wm_base->on__destroy = [=]() { wm_base->destroy(); };
    };
}

int proc(int fd, uint32_t mask, void *data) {
	lorie_compositor *b = static_cast<lorie_compositor*>(data);
	if (b == nullptr) {LOGF("b == nullptr"); return 0;}

	b->queue.run();
	return 0;
};

void lorie_compositor::start() {
	LogInit();
	LOGV("Starting compositor");
	wl_display_add_socket_auto(display);

	wl_event_loop_add_fd(wl_display_get_event_loop(display), queue.get_fd(), WL_EVENT_READABLE, &proc, this);

	wl_display_init_shm (display);

	backend_init();

	wl_display_run(display);
}

void lorie_compositor::post(std::function<void()> f) {
    queue.write(f);
}

void lorie_compositor::terminate() {
	LOGI("JNI: requested termination");
	if (display != nullptr)
		wl_display_terminate(display);
}

void lorie_compositor::output_resize(int width, int height, uint32_t physical_width, uint32_t physical_height) {
	// Xwayland segfaults without that line
	if (width == 0 || height == 0 || physical_width == 0 || physical_height == 0) return;
	renderer.resize(width, height, physical_width, physical_height);
	post([this]() {
		renderer.request_redraw();
	});
}

void lorie_compositor::report_mode(wayland::output_t* output) const {
	output->geometry(0, 0, renderer.physical_width, renderer.physical_height, output_subpixel::unknown, "Lorie", "none", output_transform::normal);
	output->scale(1.0);
	output->mode(output_mode::current | output_mode::preferred, renderer.width, renderer.height, 60000);
	output->done();
}

void lorie_compositor::touch_down(uint32_t id, uint32_t x, uint32_t y) {
    LOGV("JNI: touch down");
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	data->touch->down(next_serial(), resource_t::timestamp(), toplevel, id, x, y);
	renderer.set_cursor_visibility(false);
}

void lorie_compositor::touch_motion(uint32_t id, uint32_t x, uint32_t y) {
    LOGV("JNI: touch motion");
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	data->touch->motion(resource_t::timestamp(), id, x, y);
	renderer.set_cursor_visibility(false);
}

void lorie_compositor::touch_up(uint32_t id) {
    LOGV("JNI: touch up");
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	data->touch->up(next_serial(), resource_t::timestamp(), id);
	renderer.set_cursor_visibility(false);
}

void lorie_compositor::touch_frame() {
    LOGV("JNI: touch frame");
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	data->touch->frame();
	renderer.set_cursor_visibility(false);
}

void lorie_compositor::pointer_motion(uint32_t x, uint32_t y) {
    LOGV("JNI: pointer motion %dx%d", x, y);
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	data->pointer->motion(resource_t::timestamp(), x, y);
	data->pointer->frame();

    renderer.set_cursor_visibility(true);
    renderer.cursor_move(x, y);
}

void lorie_compositor::pointer_scroll(uint32_t axis, float value) {
    LOGV("JNI: pointer scroll %d  %f", axis, value);
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

    data->pointer->axis_discrete(pointer_axis(axis), (value >= 0) ? 1 : -1);
    data->pointer->axis(resource_t::timestamp(), pointer_axis(axis), value);
    data->pointer->frame();
	renderer.set_cursor_visibility(true);
}

void lorie_compositor::pointer_button(uint32_t button, uint32_t state) {
    LOGV("JNI: pointer button %d type %d", button, state);
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());

	LOGI("pointer button: %d %d", button, state);
	data->pointer->button(next_serial(), resource_t::timestamp(), button, pointer_button_state(state));
	data->pointer->frame();
    renderer.set_cursor_visibility(true);
}

void lorie_compositor::keyboard_key(uint32_t key, keyboard_key_state state) {
	if (!toplevel)
		return;

	auto data = any_cast<client_data*>(toplevel->client()->user_data());
	data->kbd->key (next_serial(), resource_t::timestamp(), key, keyboard_key_state(state));
}

uint32_t lorie_compositor::next_serial() const {
	if (display == nullptr) return 0;

	return wl_display_next_serial(display);
}

#pragma clang diagnostic pop
