#include <lorie-compositor.hpp>

void LorieDataDeviceManager::bind(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
	(*client)->data_device_manager.create<LorieDataDeviceManager>(client, id);
};
