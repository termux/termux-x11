#include <lorie-compositor.hpp>
#include <lorie-client.hpp>
#include <stdio.h>
#include <assert.h>

LorieClient::LorieClient(struct wl_client* client, LorieCompositor &compositor) :
	compositor(compositor), client(client) {
	notify = &destroyed;
	wl_client_add_destroy_listener(client, this);
	LOGI("Client created");
}

void LorieClient::destroyed(struct wl_listener *listener, void *data) {
	LorieClient* c = static_cast<LorieClient*>(listener);
	if (c == NULL) return;

	if (c->compositor.toplevel && c->compositor.toplevel->client == *c)
		c->compositor.set_toplevel(nullptr);
	
	if (c->compositor.cursor && c->compositor.cursor->client == *c)
		c->compositor.set_cursor(nullptr, 0, 0);
	
	LOGI("Client destroyed");
	delete c;
}

LorieClient* LorieClient::get(struct wl_client* client) {
	if (client == NULL) return NULL;
	return static_cast<LorieClient*>(wl_client_get_destroy_listener(client, &destroyed));
}

LorieClient& LorieClient::get() {
	return *this;
}
