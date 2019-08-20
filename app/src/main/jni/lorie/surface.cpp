#include <lorie-compositor.hpp>
#include <lorie-client.hpp>
#include <lorie-renderer.hpp>

void LorieSurface::request_attach(struct wl_resource *buffer, int32_t x, int32_t y) {
	if (client == nullptr) return;
	LorieRenderer* renderer = &(*client)->compositor.renderer;
	this->buffer = buffer;
	struct wl_shm_buffer *shm_buffer = wl_shm_buffer_get (buffer);
	if (shm_buffer == NULL) {
		texture.set_data(renderer, 0, 0, nullptr);
		return;
	}

	uint32_t width = (uint32_t) wl_shm_buffer_get_width (shm_buffer);
	uint32_t height = (uint32_t) wl_shm_buffer_get_height (shm_buffer);
	void *data = wl_shm_buffer_get_data(shm_buffer);

	texture.set_data(renderer, width, height, data);
}

void LorieSurface::request_damage(int32_t x, int32_t y, int32_t width, int32_t height) {
	if (client == nullptr) return;

	texture.damage(x, y, width, height);
}

void LorieSurface::request_frame(uint32_t callback) {
	frame_callback = wl_resource_create (client, &::wl_callback_interface, 1, callback);
}

void LorieSurface::request_commit() {
	if (!buffer) return;
	wl_buffer_send_release (buffer);
		
	if (frame_callback) {
		wl_callback_send_done (frame_callback, LorieUtils::timestamp());
		wl_resource_destroy (frame_callback);
		frame_callback = NULL;
	}
}

void LorieShellSurface::request_set_toplevel() {
	if (client == nullptr || surface == nullptr) return;

	(*client)->compositor.set_toplevel(surface);
	(*client)->pointer.send_enter();
	(*client)->keyboard.send_enter();
	
	send_configure(0, (*client)->compositor.renderer.width, (*client)->compositor.renderer.height);
}

