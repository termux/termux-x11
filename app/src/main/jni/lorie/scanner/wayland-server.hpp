#pragma once
#include <wayland-server.h>

class wl_resource_t {
public:
	wl_resource_t() {};

	struct wl_display *display = nullptr;
	struct wl_client *client = nullptr;
	struct wl_resource *resource = nullptr;

	bool is_valid() {
		return (display != nullptr && client != nullptr && resource != nullptr);
	};

	operator struct wl_resource*() {
		return resource;
	};
	operator struct wl_client*(){
		return client;
	};

	template<typename type>
	static void create(struct wl_client *client, uint32_t id) {
		type* res = new type;

		res->create(client, id, true);
	};

	void create(struct wl_client *client_, uint32_t id, bool free_on_destroy) {
		init();
		display = wl_client_get_display(client_);
		client = client_;
		resource = wl_resource_create(client, interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(client);
			return;
		}
		wl_resource_set_implementation(resource, implementation, this, (free_on_destroy)?annihilate:nullptr);
		on_create();
	};
	
	template<typename type>
	static void global_create(struct wl_display *display, void *data) {
		type i;
		i.init();
		wl_global_create(display, i.interface, i.version, data, bind_global<type>);
	};
	
	template<typename type>
	static void global_create(struct wl_display *display, wl_global_bind_func_t bind, void *data) {
		type i;
		i.init();
		wl_global_create(display, i.interface, i.version, data, bind);
	};

	void destroy() {
		if (is_valid()) {
			static_cast<wl_resource_t*>(wl_resource_get_user_data(resource))->on_destroy();
			wl_resource_destroy(resource);
			display = nullptr;
			resource = nullptr;
			client = nullptr;
		}
	};

	virtual void on_create() = 0;
	virtual void on_destroy() = 0;
	
	virtual void init() = 0;
	
	static void annihilate(struct wl_resource* resource) {
		wl_resource_t *res = static_cast<wl_resource_t*>(wl_resource_get_user_data(resource));
		if (res != nullptr) delete res;
	};
	uint32_t next_serial() {
		if (display == nullptr) return 0;
		return wl_display_next_serial(display);
	};

	template<typename type>
	static type* from_wl_resource(struct wl_resource *resource) {
		if (resource == nullptr) return static_cast<type*>(nullptr);
		return static_cast<type*>(wl_resource_get_user_data(resource));
	};

	virtual ~wl_resource_t() {};
private:
	void non_virtual_init() { init(); };
	
	template <typename type>
	static void bind_global(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
		create<type>(client, id);
	};

protected:
	uint32_t version;
	const void *implementation;
	const struct wl_interface *interface;
};


struct wl_client_t: public wl_listener {
public:
	struct wl_client* operator ()() {
		return client;
	};
	virtual void on_destroy() = 0;
	virtual ~wl_client_t();
private:
	struct wl_client *client = nullptr;
	static void destroyed_callback(struct wl_listener *listener, void *data) {
		wl_client_t *c = static_cast<wl_client_t*>(listener);
		if (c == nullptr) return;
		
		c->on_destroy();
		delete c;
	};
};
