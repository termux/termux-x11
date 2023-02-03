#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-static-cast-downcast"
#pragma once
 
#include "wayland-server.h"
#include <functional>
#include <algorithm>
#include <type_traits>
#include <any>
#include <iostream>
#include <cstring>

namespace wayland {
    class client_t;
	class display_t: public wl_listener {
    private:
		wl_display *display;
        wl_listener client_created_listener;
		
		public:
        display_t();
        std::function<void(client_t*)> on_client = nullptr;
        std::function<void()> on_destroy = nullptr;

        inline void terminate() {
            wl_display_terminate(*this);
        }
		
		inline operator wl_display*() {
			return display;
		}

        ~display_t();

    public:
        void add_fd_listener(int fd, uint32_t mask, std::function<int(int fd, uint32_t mask)> listener);
		
		void add_socket_fd(int fd);

		inline void add_socket_auto() {
			wl_display_add_socket_auto(display);
		}

		inline void adopt_client(int fd) {
			auto r = wl_client_create(display, fd);
		}
		
		inline void init_shm() {
			wl_display_init_shm(display);
		}
		
		inline void run() {
			wl_display_run(display);
		}

		inline void set_log_handler(wl_log_func_t f) {
			wl_log_set_handler_server(f);
		}

		inline uint32_t next_serial() {
			return wl_display_next_serial(*this);
		};
	};

    class client_t: public wl_listener {
    protected:
        std::any userdata;
    public:
        std::function<void()> on_destroy = nullptr;
        explicit client_t(wl_client* client);

        inline operator wl_client*() {
            return client;
        }

        static client_t* get(wl_client*);

        std::any& user_data() {
            return userdata;
        }

        void post_implementation_error(const std::string& string);

        void destroy();

        inline bool operator==(const client_t& other) {
            return client == other.client;
        }

        inline bool operator!=(const client_t& other) {
            return client != other.client;
        }

    private:
        struct wl_client *client;
    };

	class resource_t: private wl_listener {
		protected:
        client_t *m_client;
        wl_display* display;
        wl_resource *resource;
        uint32_t version;
        static void resource_destroyed(wl_listener* that, void*);
		
        std::any userdata;

		resource_t(client_t* client, uint32_t id, uint32_t version, wl_interface* iface, wl_dispatcher_func_t dispatcher);
	public:
        explicit resource_t(wl_resource *r); // wl_buffer is a builtin interface...

		public:
		std::function<void()> on_destroy = nullptr;
		
		[[nodiscard]] inline bool is_valid() const {
			return (display != nullptr && m_client != nullptr && resource != nullptr);
		};

        wl_resource* c_ptr() {
            resource_t *r = this;
            return r == nullptr? nullptr : resource;
        };

        inline operator wl_client*() const {
            return *m_client;
        };

        inline operator wl_resource*() const {
            return resource;
        };

        inline client_t* client() {
            return m_client;
        }
		
		static inline resource_t* get(wl_resource* r) {
			return r == nullptr ? nullptr : static_cast<resource_t*>(wl_resource_get_destroy_listener(r, &resource_destroyed));
		};
		
		static inline resource_t* get(void* r) {
			return get(static_cast<wl_resource*>(r));
		};
		
		std::any& user_data() {
            return userdata;
        }

        inline void destroy() {
            wl_resource_destroy(*this);
        }
		
		inline void post_error(uint32_t code, const std::string& msg) const {
			wl_resource_post_error(resource, code, "%s", msg.c_str());
		};

        static uint32_t timestamp();
		
        virtual ~resource_t() = default;
    };

    template<typename T>
	class global_t {
		wl_global* global;
		static void bind(wl_client *client, void *data, uint32_t version, uint32_t id) {
			auto g = static_cast<global_t*>(data);
            auto c = client_t::get(client);
			T* t = new T(c, id, std::min(version, T::max_version));
			if (g) g->on_bind(c, t);
		};
		
		public:
		inline explicit global_t(display_t* display):
		global(wl_global_create(*display, T::interface, T::max_version, this, &global_t::bind)) {}
        inline explicit global_t(display_t& display): global_t(&display){}
		
		std::function<void(client_t*, T*)> on_bind = nullptr;
	};

    class buffer_t : public resource_t {
        public:
        explicit buffer_t(wl_resource *r): resource_t(r) {}

        operator wl_buffer *() const {
            return reinterpret_cast<wl_buffer *>(const_cast<buffer_t*>(this));
        };

        inline void release() {
            wl_resource_post_event(resource, 0);
        }

        [[nodiscard]]
        inline bool is_shm() {
            return wl_shm_buffer_get(*this) != nullptr;
        }

        [[nodiscard]]
        inline void *shm_data() const {
        return wl_shm_buffer_get_data(
            wl_shm_buffer_get(*this));
        }

        [[nodiscard]]
        inline int32_t shm_stride() const {
            return wl_shm_buffer_get_stride(wl_shm_buffer_get(*this));
        }

        [[nodiscard]]
        inline uint32_t shm_format() const {
            return wl_shm_buffer_get_format(wl_shm_buffer_get(*this));
        }

        [[nodiscard]]
        inline int32_t shm_width() const {
            return wl_shm_buffer_get_width(wl_shm_buffer_get(*this));
        }

        [[nodiscard]]
        inline int32_t shm_height() const {
            return wl_shm_buffer_get_height(wl_shm_buffer_get(*this));
        }
    };
}

#pragma clang diagnostic pop
