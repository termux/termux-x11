#include <sys/ioctl.h>
#include <iostream>
#include <thread>
#include <xcb/xcb.h>
#include <xcb/damage.h>
#include <xcb/xinput.h>
#include <xcb/xfixes.h>
#include <xcb/shm.h>
#include <xcb_errors.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <cstring>
#include <jni.h>
#include <android/log.h>
#include <linux/un.h>
#include <sys/stat.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#pragma ide diagnostic ignored "ConstantParameter"

#if 1
#define ALOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "LorieX11Client", fmt, ## __VA_ARGS__)
#else
#define ALOGE(fmt, ...) printf(fmt, ## __VA_ARGS__); printf("\n")
#endif
#define unused __attribute__((unused))

typedef uint8_t u8 unused;
typedef uint16_t u16 unused;
typedef uint32_t u32 unused;
typedef uint64_t u64 unused;
typedef int8_t i8 unused;
typedef int16_t i16 unused;
typedef int32_t i32 unused;
typedef int64_t i64 unused;

#define always_inline inline __attribute__((__always_inline__))

#define xcb(name, ...) xcb_ ## name ## _reply(self.conn, xcb_ ## name(self.conn, ## __VA_ARGS__), &self.err)
#define xcb_check(name, ...) self.err = xcb_request_check(self.conn, xcb_ ## name(self.conn, ## __VA_ARGS__))

class xcb_connection {
private:
    xcb_connection_t *conn{};

public:
    xcb_generic_error_t* err{}; // not thread-safe, but the whole class should be used in separate thread
    xcb_errors_context_t *err_ctx{};
    template<typename REPLY>
    always_inline void handle_error(REPLY* reply, std::string description) { // NOLINT(performance-unnecessary-value-param)
        if (err) {
            const char* ext{};
            const char* err_name =  xcb_errors_get_name_for_error(err_ctx, err->error_code, &ext);
            std::string err_text = description + "\n" +
                                   "XCB Error of failed request:               " + (ext?:"") + "::" + err_name + "\n" +
                                   "  Major opcode of failed request:          " + std::to_string(err->major_code) + " (" +
                                   xcb_errors_get_name_for_major_code(err_ctx, err->major_code) + ")\n" +
                                   "  Minor opcode of failed request:          " + std::to_string(err->minor_code) + " (" +
                                   xcb_errors_get_name_for_minor_code(err_ctx, err->major_code, err->minor_code) + ")\n" +
                                   "  Serial number of failed request:         " + std::to_string(err->sequence) + "\n" +
                                   "  Current serial number in output stream:  " + std::to_string(err->full_sequence);

            free(reply);
            free(err);
            err = nullptr;
            throw std::runtime_error(err_text);
        }
    }

    always_inline void handle_error(std::string description) { // NOLINT(performance-unnecessary-value-param)
        if (err) {
            const char* ext{};
            const char* err_name =  xcb_errors_get_name_for_error(err_ctx, err->error_code, &ext);
            std::string err_text = description + "\n" +
                                   "XCB Error of failed request:               " + (ext?:"") + "::" + err_name + "\n" +
                                   "  Major opcode of failed request:          " + std::to_string(err->major_code) + " (" +
                                   xcb_errors_get_name_for_major_code(err_ctx, err->major_code) + ")\n" +
                                   "  Minor opcode of failed request:          " + std::to_string(err->minor_code) + " (" +
                                   xcb_errors_get_name_for_minor_code(err_ctx, err->major_code, err->minor_code) + ")\n" +
                                   "  Serial number of failed request:         " + std::to_string(err->sequence) + "\n" +
                                   "  Current serial number in output stream:  " + std::to_string(err->full_sequence);
            free(err);
            err = nullptr;
            throw std::runtime_error(err_text);
        }
    }

    struct {
        xcb_connection& self;
        i32 first_event{};
        void init() {
            {
                auto reply = xcb(shm_query_version);
                self.handle_error(reply, "Error querying MIT-SHM extension");
                free(reply);
            }
            {
                auto reply = xcb(query_extension, 6, "DAMAGE");
                self.handle_error(reply, "Error querying DAMAGE extension");
                first_event = reply->first_event;
                free(reply);
            }
        };

        void attach_fd(u32 seg, i32 fd, u8 ro) {
            xcb_check(shm_attach_fd, seg, fd, ro);
            self.handle_error("Error attaching file descriptor through MIT-SHM extension");
        };

        void unused detach(u32 seg) {
            xcb_check(shm_detach, seg);
            self.handle_error("Error attaching shared segment through MIT-SHM extension");
        }

        xcb_shm_get_image_reply_t* get(xcb_drawable_t d, i16 x, i16 y, i16 w, i16 h, u32 m, u8 f, xcb_shm_seg_t s, u32 o) {
            auto reply = xcb(shm_get_image, d, x, y, w, h, m, f, s, o);
            self.handle_error(reply, "Error getting shm image through MIT-SHM extension");
            return reply;
        };

        void put() {

        }
    } shm {*this};

    struct {
        xcb_connection& self;
        i32 first_event{};

        void init() {
            {
                auto reply = xcb(query_extension, 6, "DAMAGE");
                self.handle_error(reply, "Error querying DAMAGE extension");
                first_event = reply->first_event;
                free(reply);
            }
            {
                auto reply = xcb(damage_query_version, 1, 1);
                self.handle_error(reply, "Error querying DAMAGE extension");
                free(reply);
            }
        }

        void create(xcb_drawable_t d, uint8_t l) {
            xcb_check(damage_create, xcb_generate_id(self.conn), d, l);
            self.handle_error("Error creating damage");
        }

        inline bool is_damage_notify_event(xcb_generic_event_t *ev) const {
            return ev->response_type == (first_event + XCB_DAMAGE_NOTIFY);
        }
    } damage {*this};

    struct {
        xcb_connection& self;
        i32 opcode{};
        void init() {
            {
                auto reply = xcb(query_extension, 15, "XInputExtension");
                self.handle_error(reply, "Error querying XInputExtension extension");
                opcode = reply->major_opcode;
                free(reply);
            }
            {
                auto reply = xcb(input_get_extension_version, 15, "XInputExtension");
                self.handle_error(reply, "Error querying XInputExtension extension");
                free(reply);
            }
            {
                auto reply = xcb(input_xi_query_version, 2, 2);
                self.handle_error(reply, "Error querying XInputExtension extension");
                free(reply);
            }
        }

        xcb_input_device_id_t client_pointer_id() {
            xcb_input_device_id_t id;
            auto reply = xcb(input_xi_get_client_pointer, XCB_NONE);
            self.handle_error(reply, "Error getting client pointer device id");
            id = reply->deviceid;
            free(reply);
            return id;
        }

        void select_events(xcb_window_t window, uint16_t num_mask, const xcb_input_event_mask_t *masks){
            xcb_check(input_xi_select_events, window, num_mask, masks);
            self.handle_error("Error selecting Xi events");
        }

        inline bool is_raw_motion_event(xcb_generic_event_t *ev) const {
            union { // NOLINT(cppcoreguidelines-pro-type-member-init)
                xcb_generic_event_t *event;
                xcb_ge_event_t *ge;
            };
            event = ev;
            return ev->response_type == XCB_GE_GENERIC && /* cookie */ ge->pad0 == opcode && ge->event_type == XCB_INPUT_RAW_MOTION;
        }
    } input {*this};

    struct {
        xcb_connection& self;
        int first_event{};
        void init() {
            {
                auto reply = xcb(query_extension, 6, "XFIXES");
                self.handle_error(reply, "Error querying XFIXES extension");
                first_event = reply->first_event;
                free(reply);
            }
            {
                auto reply = xcb(xfixes_query_version, 4, 0);
                self.handle_error(reply, "Error querying XFIXES extension");
                free(reply);
            }
        }

        void select_input(xcb_window_t window, uint32_t mask) {
            xcb_check(xfixes_select_cursor_input, window, mask);
            self.handle_error("Error querying selecting XFIXES input");
        }

        bool is_cursor_notify_event(xcb_generic_event_t* e) const {
            return e->response_type == first_event + XCB_XFIXES_CURSOR_NOTIFY;
        }

        xcb_xfixes_get_cursor_image_reply_t* unused get_cursor_image() {
            auto reply = xcb(xfixes_get_cursor_image);
            self.handle_error(reply, "Error getting XFIXES cursor image");
            return reply;
        }
    } fixes {*this};

    void init(xcb_connection_t* new_conn) {
        if (err_ctx)
            xcb_errors_context_free(err_ctx);
        if (conn)
            xcb_disconnect(conn);

        int conn_err = xcb_connection_has_error(new_conn);
        if (conn_err) {
            const char *s;
            switch (conn_err) {
#define c(name) case name: s = static_cast<const char*>(#name); break
                c(XCB_CONN_ERROR);
                c(XCB_CONN_CLOSED_EXT_NOTSUPPORTED);
                c(XCB_CONN_CLOSED_MEM_INSUFFICIENT);
                c(XCB_CONN_CLOSED_REQ_LEN_EXCEED);
                c(XCB_CONN_CLOSED_PARSE_ERR);
                c(XCB_CONN_CLOSED_INVALID_SCREEN);
                c(XCB_CONN_CLOSED_FDPASSING_FAILED);
                default:
                    s = "UNKNOWN";
#undef c
            }
            throw std::runtime_error(std::string() + "XCB connection has error: " + s);
        }

        conn = new_conn;
        xcb_errors_context_new(conn, &err_ctx);

        shm.init();
        damage.init();
        input.init();
        fixes.init();
    }
};



#define ASHMEM_SET_SIZE _IOW(0x77, 3, size_t)

static inline int
os_create_anonymous_file(size_t size) {
    int fd, ret;
    long flags;
    fd = open("/dev/ashmem", O_RDWR | O_CLOEXEC);
    if (fd < 0)
        return fd;
    ret = ioctl(fd, ASHMEM_SET_SIZE, size);
    if (ret < 0)
        goto err;
    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
        goto err;
    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
        goto err;
    return fd;
    err:
    close(fd);
    return ret;
}

ANativeWindow* win{};

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_surface(JNIEnv *env, jobject thiz, jobject sfc) {
    if (win)
        ANativeWindow_release(win);

    win = sfc ? ANativeWindow_fromSurface(env, sfc) : nullptr;
    if (win)
        ANativeWindow_acquire(win);
    ALOGE("Got new surface %p", win);
}

// For some reason both static_cast and reinterpret_cast returning 0 when casting b.bits.
static always_inline uint32_t* cast(void* p) { union { void* a; uint32_t* b; } c {p}; return c.b; } // NOLINT(cppcoreguidelines-pro-type-member-init)

static always_inline void blit_exact(ANativeWindow* win, const uint32_t* src, int width, int height) {
    if (width == 0 || height == 0) {
        width = ANativeWindow_getWidth(win);
        height = ANativeWindow_getHeight(win);
    }
    ARect bounds{ 0, 0, width, height };
    ANativeWindow_Buffer b{};

    ANativeWindow_acquire(win);
    auto ret = ANativeWindow_setBuffersGeometry(win, width, height, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);
    if (ret != 0) {
        ALOGE("Failed to set buffers geometry (%d)", ret);
        return;
    }

    ret = ANativeWindow_lock(win, &b, &bounds);
    if (ret != 0) {
        ALOGE("Failed to lock");
        return;
    }

    uint32_t* dst = cast(b.bits);
    if (src) {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                uint32_t s = src[width * i + j];
                // Cast BGRA to RGBA
                dst[b.stride * i + j] = (s & 0xFF000000) | ((s & 0x00FF0000) >> 16) | (s & 0x0000FF00) | ((s & 0x000000FF) << 16);
            }
        }
    } else
        memset(dst, 0, b.stride*b.height);

    ret = ANativeWindow_unlockAndPost(win);
    if (ret != 0) {
        ALOGE("Failed to post");
        return;
    }

    ANativeWindow_release(win);
}

static void thread_proc(int sockfd[1]) {
    if (!win) {
        ALOGE("No surface to draw");
        return;
    }
    ALOGE("Connecting to fd %d", sockfd[0]);
    xcb_connection_t* conn = xcb_connect_to_fd(sockfd[0], nullptr);
    xcb_connection c;
    c.init(conn);
    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

    xcb_change_window_attributes(conn, screen->root, XCB_CW_EVENT_MASK, (const int[]) { XCB_EVENT_MASK_STRUCTURE_NOTIFY });
    c.fixes.select_input(screen->root, XCB_XFIXES_CURSOR_NOTIFY_MASK_DISPLAY_CURSOR);
    c.damage.create(screen->root, XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);
    struct {
        xcb_input_event_mask_t head;
        xcb_input_xi_event_mask_t mask;
    } mask{};
    mask.head.deviceid = c.input.client_pointer_id();
    mask.head.mask_len = sizeof(mask.mask) / sizeof(uint32_t);
    mask.mask = XCB_INPUT_XI_EVENT_MASK_RAW_MOTION;
    c.input.select_events(screen->root, 1, &mask.head);

    xcb_shm_seg_t shmseg = xcb_generate_id(conn);

#if 0
    auto seg = xcb_shm_create_segment_reply(conn, xcb_shm_create_segment(conn, shmseg, 8096*8096*4, 0), &c.err);
    c.handle_error(seg, "Error getting fds");
    int shmfd = xcb_shm_create_segment_reply_fds(conn, seg)[0];
    ALOGE("Got fd %d", shmfd);
#else
    ALOGE("Creating file...");
    int shmfd = os_create_anonymous_file(8096*8096*4);
    if (shmfd < 1) {
        ALOGE("Error opening file: %s", strerror(errno));
    }
    fchmod(shmfd, 0777);
    ALOGE("Attaching file...");
    unused u32 *shmaddr = static_cast<u32 *>(mmap(nullptr, 8096*8096*4,
                                                    PROT_READ | PROT_WRITE,
                                                    MAP_SHARED, shmfd, 0));
    if (shmaddr == MAP_FAILED) {
        ALOGE("Map failed: %s", strerror(errno));
    }
    c.shm.attach_fd(shmseg, shmfd, 0);
#endif
    xcb_generic_event_t *event;
    const char* ext;

    while((event = xcb_wait_for_event(conn))) {
        if (!event) {
            ALOGE("No event...");
            usleep(10000);
            continue;
        }
        if (event->response_type == 0) {
            c.err = reinterpret_cast<xcb_generic_error_t *>(event);
            c.handle_error("Error processing XCB events");
        } else if (event->response_type == XCB_CONFIGURE_NOTIFY) {
            ALOGE("Configure notification. ");
            auto e = reinterpret_cast<xcb_configure_request_event_t *>(event);
            ALOGE("old w: %d h: %d",screen->width_in_pixels, screen->height_in_pixels);
            ALOGE("new w: %d h: %d",e->width, e->height);
            screen->width_in_pixels = e->width;
            screen->height_in_pixels = e->height;
        } else if (c.damage.is_damage_notify_event(event)) {
            try {
                c.shm.get(screen->root, 0, 0, screen->width_in_pixels, screen->height_in_pixels, ~0, // NOLINT(cppcoreguidelines-narrowing-conversions)
                          XCB_IMAGE_FORMAT_Z_PIXMAP, shmseg, 0);

                blit_exact(win, shmaddr, screen->width_in_pixels, screen->height_in_pixels);
            } catch (std::runtime_error &err) {
                continue;
            }
        } //else
          //  ALOGE("some other event %s of %s", xcb_errors_get_name_for_core_event(c.err_ctx, event->response_type, &ext), (ext ?: "core"));
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_start(unused JNIEnv *env, unused jobject thiz, jint fd) {
    pthread_t t1;
    int res = pthread_create(&t1, nullptr, reinterpret_cast<void *(*)(void *)>(thread_proc), &fd);
    if (res)
    {
        printf ("error %d\n", res);
    }
    sleep(1);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_termux_x11_CmdEntryPoint_connect(JNIEnv *env, jclass clazz) {
    struct sockaddr_un remote{ .sun_family=AF_UNIX, .sun_path="/data/data/com.termux/files/usr/tmp/.X11-unix/X0" };
    int sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    std::cerr << "Connecting..." << std::endl;
    connect(sockfd, reinterpret_cast<const sockaddr *>(&remote), sizeof(remote)); // NOLINT(bugprone-unused-return-value)
    return sockfd;
}

// g++ ss.cpp -o ss -I$PREFIX/include/xcb -lxcb -lxcb-shm -lxcb-damage -lxcb-errors -lxcb-xfixes -lxcb-xinput

int main() {
    Java_com_termux_x11_CmdEntryPoint_connect(nullptr, nullptr);
}