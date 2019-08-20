#include <LorieImpls.hpp>
#include <lorie-compositor.hpp>
#include <lorie-client.hpp>
#include <unistd.h>

void LorieSeat::on_create() {
	uint32_t capabilities = (*client)->compositor.input_capabilities();
	send_capabilities (capabilities);
	send_name("default");
}

void LorieSeat::request_get_pointer(uint32_t id) {
	LOGV("Client requested seat pointer");
	if (client == nullptr) return;
	(*client)->pointer.create(client, id, false);
}

void LorieSeat::request_get_keyboard(uint32_t id) {
	LOGV("Client requested seat keyboard");
	if (client == nullptr) return;
	(*client)->keyboard.create(client, id, false);
}

void LorieSeat::request_get_touch(uint32_t id) {
	LOGV("Client requested seat touch");
	if (client == nullptr) return;
	(*client)->touch.create(client, id, false);
}

void LoriePointer::on_create() {
	send_enter();
}

void LoriePointer::request_set_cursor(uint32_t serial,
					     struct wl_resource *_surface,
					     int32_t hotspot_x,
					     int32_t hotspot_y) {
	if (client == nullptr) return;

	LorieSurface *surface = LorieSurface::from_wl_resource<LorieSurface>(_surface);
	(*client)->compositor.set_cursor(surface, (uint32_t) hotspot_x, (uint32_t) hotspot_y);
}

void LoriePointer::send_enter() {
	if (client == nullptr || (*client)->compositor.toplevel == nullptr) return;

	wl_fixed_t x = wl_fixed_from_double ((*client)->compositor.toplevel->texture.width/2);
	wl_fixed_t y = wl_fixed_from_double ((*client)->compositor.toplevel->texture.height/2);
	wl_pointer_t::send_enter(next_serial(), *(*client)->compositor.toplevel, x, y);
}

void LorieKeyboard::on_create() {
	keymap_changed();
	send_enter();
}

void LorieKeyboard::send_enter() {
	if (client == nullptr || (*client)->compositor.toplevel == nullptr) return;

	struct wl_array keys;
	wl_array_init(&keys);
	wl_keyboard_t::send_enter(next_serial(), *(*client)->compositor.toplevel, &keys);
}

void LorieKeyboard::keymap_changed() {
	if (client == nullptr) return;
	
	int fd = -1, size = -1;
	
	(*client)->compositor.get_keymap(&fd, &size);
	if (fd == -1 || size == -1) {
		LOGE("Error while getting keymap from backend");
		return;
	}

	send_keymap(WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd, size);
	close (fd);
}
