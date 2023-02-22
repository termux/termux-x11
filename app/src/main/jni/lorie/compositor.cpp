#include <lorie_compositor.hpp>
#include <unistd.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

using namespace wayland;

lorie_compositor::lorie_compositor() {
    on_client = [=](client_t* client) {
		client->user_data() = new client_data;
        client->on_destroy = [=] {
            bool request_redraw{};

            if (screen.sfc && screen.sfc->client() == client) {
                screen.sfc = nullptr;
                request_redraw = true;
            }

            if (cursor.sfc && cursor.sfc->client() == client) {
                screen.sfc = nullptr;
                request_redraw = true;
            }

            if (request_redraw)
                redraw(true);

            if(!screen.sfc)
                set_renderer_visibility(false);
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
            data->id = surface->id(); // For debugging purposes
            surface->user_data() = data;
            surface->on_attach = [=](buffer_t* b, int32_t, int32_t) {
                data->buffer = b;
            };
            surface->on_damage = [=](int32_t, int32_t, int32_t, int32_t) {
                data->damaged = true;
            };
            surface->on_damage_buffer = [=](int32_t, int32_t, int32_t, int32_t) {
                data->damaged = true;
            };
            surface->on_frame = [=] (callback_t* cb) {
                data->frame_callback = cb;
            };
            surface->on_commit = [=] {
                redraw();
                if (data->buffer)
                    data->buffer->release();
                if (data->frame_callback) {
                    data->frame_callback->done(resource_t::timestamp());
                    data->frame_callback = nullptr;
                }
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

        seat->on_get_pointer = [=, this](pointer_t* pointer) {
            LOGV("Client requested seat pointer");
            data->pointer = pointer;
            if (screen.sfc)
                pointer->enter(next_serial(), screen.sfc, 0, 0);

            pointer->on_set_cursor = [=](uint32_t, wayland::surface_t* sfc, int32_t x, int32_t y) {
                cursor.sfc = sfc;
                cursor.hotspot_x = x;
                cursor.hotspot_y = y;

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

            if (screen.sfc)
                kbd->enter(next_serial(), screen.sfc, &keys);

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
                screen.sfc = sfc;
                redraw();
                set_renderer_visibility(sfc != nullptr);

                if (data->pointer)
                    data->pointer->enter(next_serial(),sfc, 0, 0);

                if(data->kbd)
                    data->kbd->enter(next_serial(), sfc, &keys);

                auto buffer = sfc ? any_cast<surface_data*>(sfc->user_data())->buffer : nullptr;
                if (buffer)
                    shell->configure(shell_surface_resize::none, buffer->shm_width(), buffer->shm_height());
            };
        };
    };
    global_xdg_wm_base.on_bind = [=, this](client_t* client, xdg_wm_base_t* wm_base) {
        auto data = any_cast<client_data*>(client->user_data());
        wm_base->on_get_xdg_surface = [=, this](xdg_surface_t* xdg_surface, surface_t* sfc) {
            xdg_surface->on_get_toplevel = [=, this](xdg_toplevel_t*) {
                wl_array keys{};
                wl_array_init(&keys);
                screen.sfc = sfc;
                redraw();
                set_renderer_visibility(sfc != nullptr);

                if (data->pointer)
                    data->pointer->enter(next_serial(),sfc, 0, 0);

                if(data->kbd)
                    data->kbd->enter(next_serial(), sfc, &keys);
            };
        };
        wm_base->on__destroy = [=]() { wm_base->destroy(); };
    };

    LOGV("Starting compositor");
    wl_display_init_shm (*this);
    add_fd_listener(queue.get_fd(), WL_EVENT_READABLE, [&](int, uint){ queue.run(); return 0; });
}

void lorie_compositor::redraw(bool force) {
    if (screen.win) {
        auto data = screen.sfc ? any_cast<surface_data *>(screen.sfc->user_data()) : nullptr;
        bool damaged = (data && data->damaged) || force;
        if (damaged)
            blit(screen.win, screen.sfc);
        if (data)
            data->damaged = false;
    }

    if (cursor.win) {
        auto data = cursor.sfc ? any_cast<surface_data *>(cursor.sfc->user_data()) : nullptr;
        bool damaged = (data && data->damaged) || force;
        if (damaged)
            blit(cursor.win, cursor.sfc);
        if (data)
            data->damaged = false;
    }
}

void lorie_compositor::post(std::function<void()> f) {
    queue.write(std::move(f));
}

void lorie_compositor::output_resize(EGLNativeWindowType win, int real_width, int real_height, int physical_width, int physical_height) {
	// Xwayland segfaults without that line
    LOGV("JNI: window is changed: %p %dx%d (%dmm x %dmm)", win, real_width, real_height, physical_width, physical_height);
	if (real_width == 0 || real_height == 0 || physical_width == 0 || physical_height == 0) return;
    screen.real_width = real_width;
    screen.real_height = real_height;
    screen.physical_width = physical_width;
    screen.physical_height = physical_height;
    screen.win = win;

    if (screen.sfc) {
        auto data = any_cast<client_data*>(screen.sfc->client()->user_data());
        report_mode(data->output);
    }
}

void lorie_compositor::report_mode(wayland::output_t* output) const {
	output->geometry(0, 0, screen.physical_width, screen.physical_height, output_subpixel::unknown, "Lorie", "none", output_transform::normal);
	output->scale(1.0);
	output->mode(output_mode::current | output_mode::preferred, screen.real_width, screen.real_height, 60000);
	output->done();
}

void lorie_compositor::pointer_motion(int x, int y) {
    LOGV("JNI: pointer motion %dx%d", x, y);
	if (!screen.sfc)
		return;

	auto data = any_cast<client_data*>(screen.sfc->client()->user_data());

	data->pointer->motion(resource_t::timestamp(), double(x), double(y));
	data->pointer->frame();
}

void lorie_compositor::pointer_scroll(int axis, float value) {
    LOGV("JNI: pointer scroll %d  %f", axis, value);
	if (!screen.sfc)
		return;

	auto data = any_cast<client_data*>(screen.sfc->client()->user_data());

    data->pointer->axis_discrete(pointer_axis(axis), (value >= 0) ? 1 : -1);
    data->pointer->axis(resource_t::timestamp(), pointer_axis(axis), value);
    data->pointer->frame();
}

void lorie_compositor::pointer_button(int button, int state) {
    LOGV("JNI: pointer button %d type %d", button, state);
	if (!screen.sfc)
		return;

	auto data = any_cast<client_data*>(screen.sfc->client()->user_data());

	LOGI("pointer button: %d %d", button, state);
	data->pointer->button(next_serial(), resource_t::timestamp(), button, pointer_button_state(state));
	data->pointer->frame();
}

void lorie_compositor::keyboard_key(uint32_t key, keyboard_key_state state) {
	if (!screen.sfc)
		return;

	auto data = any_cast<client_data*>(screen.sfc->client()->user_data());
	data->kbd->key (next_serial(), resource_t::timestamp(), key, keyboard_key_state(state));
}

#pragma clang diagnostic pop
