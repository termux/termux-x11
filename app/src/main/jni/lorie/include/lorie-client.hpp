#pragma once
#include <LorieImpls.hpp>

class LorieCompositor;
class LoriePointer;
class LorieKeyboard;
class LorieTouch;
class LorieSeat;
class LorieOutput;
class LorieClient : public wl_listener {
public:
	LorieClient(struct wl_client* client, LorieCompositor &compositor);

	LorieClient& get();
	static LorieClient* get(struct wl_client* client);
	LorieOutput *output = NULL;
	LorieCompositor& compositor;
	LoriePointer pointer;
	LorieKeyboard keyboard;
	LorieTouch touch;
	LorieSeat seat;
	LorieDataDeviceManager data_device_manager;
	operator struct wl_client*() { return client; }
private:
	struct wl_client *client = NULL;
	
	static void destroyed(struct wl_listener *listener, void *data);
};

template <typename client_t, typename data_t>
class wl_client_created_listener_t: public wl_listener {
public:
	wl_client_created_listener_t(data_t user_data):user_data(user_data){
		wl_listener::notify = &created_callback;
	};
private:
	data_t user_data = nullptr;
	static void created_callback(struct wl_listener* listener, void *data) {
		if (listener == nullptr || data == nullptr) return;
		
		new client_t(static_cast<struct wl_client*>(data), 
					(static_cast<wl_client_created_listener_t*>(listener))->user_data);
	}
};

struct wl_client {
public:
	LorieClient* operator ->() {
		return LorieClient::get(this);
	}
	operator LorieClient*() {
		return LorieClient::get(this);
	}
};
