#include <lorie-compositor.hpp>
#include <lorie-client.hpp>
#include <lorie-renderer.hpp>

void LorieSurface::request_attach(struct wl_resource *wl_buffer, int32_t x, int32_t y) {
	if (client == nullptr) return;
	buffer = wl_buffer;
	struct wl_shm_buffer *shm_buffer = wl_shm_buffer_get (buffer);
	if (shm_buffer == NULL) {
		width = height = 0;
		return;
	}

	width = (uint32_t) wl_shm_buffer_get_width (shm_buffer);
	height = (uint32_t) wl_shm_buffer_get_height (shm_buffer);
	
	if (width == 0 || height == 0) {
		texture.uninit();
		return;
	}

	if (!texture.valid() || 
		texture.width != width || 
		texture.height != height) {
		texture.reinit(&(*client)->compositor.renderer, width, height);
	}
}

void LorieSurface::request_damage(int32_t x, int32_t y, int32_t width, int32_t height) {
	if (client == nullptr) return;
	
	(*client)->compositor.renderer.requestRedraw();
}

void LorieSurface::request_frame(uint32_t callback) {
	frame_callback = wl_resource_create (client, &::wl_callback_interface, 1, callback);
}

void LorieSurface::request_commit() {
	if (!buffer) return;
	struct wl_shm_buffer *shm_buffer = wl_shm_buffer_get (buffer);
	if (texture.valid() && shm_buffer) {
		void *data = wl_shm_buffer_get_data (shm_buffer);
		if (data) {
			texture.upload(data);
		}
	}
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

