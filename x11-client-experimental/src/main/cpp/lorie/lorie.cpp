#include <sys/ioctl.h>
#include <iostream>
#include <thread>
#include <functional>
#include <xcb/xcb.h>
#include <xcb/damage.h>
#include <xcb/xinput.h>
#include <xcb/xfixes.h>
#include <xcb/shm.h>
#include <xcb/randr.h>
#include <xcb_errors.h>
#include <libxcvt/libxcvt.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <cstring>
#include <jni.h>
#include <android/log.h>
#include <linux/un.h>
#include <sys/stat.h>
#include <android/native_window_jni.h>
#include <android/looper.h>
#include "lorie_message_queue.hpp"

// To avoid reopening new segment on every screen resolution
// change we can open it only once with some maximal size
#define DEFAULT_SHMSEG_LENGTH 8192*8192*4

#pragma ide diagnostic ignored "ConstantParameter"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"

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

public:
    xcb_connection_t *conn{};
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

    struct {
        xcb_connection& self;
        xcb_randr_get_screen_resources_reply_t* res{};
        const char* temporary_name = "temporary";
        void init() {
            {
                auto reply = xcb(randr_query_version, 1, 1);
                self.handle_error(reply, "Error querying RANDR extension");
                free(reply);
            }

            refresh();
        }

        void refresh() {
            auto screen = xcb_setup_roots_iterator(xcb_get_setup (self.conn)).data;
            free(res);
            res = xcb(randr_get_screen_resources, screen->root);
            self.handle_error(res, "Error during refreshing RANDR modes.");
        }

        xcb_randr_mode_t get_id_for_mode(const char *name) {
            refresh();
            char *mode_names = reinterpret_cast<char*>(xcb_randr_get_screen_resources_names(res));
            auto modes = xcb_randr_get_screen_resources_modes(res);
            for (int i = 0; i < xcb_randr_get_screen_resources_modes_length(res); i++) {
                auto& mode = modes[i];
                char mode_name[64]{};
                snprintf(mode_name, mode.name_len+1, "%s", mode_names);
                mode_names += mode.name_len;

                if (!strcmp(mode_name, name)) {
                    return mode.id;
                }
            }
            return 0;
        }

        void create_mode(const char* name, libxcvt_mode_info *mode_info) {
            bool is_temporary = strcmp(name, temporary_name) == 0;
            if (!is_temporary && get_id_for_mode(name))
                delete_mode(name);

            if (is_temporary && get_id_for_mode(name))
                return;

            auto screen = xcb_setup_roots_iterator(xcb_get_setup (self.conn)).data;
            xcb_randr_mode_info_t mode{};
            mode.width = mode_info->hdisplay;
            mode.height = mode_info->vdisplay;
            mode.dot_clock = mode_info->dot_clock * 1000;
            mode.hsync_start = mode_info->hsync_start;
            mode.hsync_end = mode_info->hsync_end;
            mode.htotal = mode_info->htotal;
            mode.vsync_start = mode_info->vsync_start;
            mode.vsync_end = mode_info->vsync_end;
            mode.vtotal = mode_info->vtotal;
            mode.mode_flags = mode_info->mode_flags;
            mode.name_len = strlen(name);
            {
                auto reply = xcb(randr_create_mode, screen->root, mode, mode.name_len, name);
                self.handle_error(reply, "Failed to create RANDR mode");
            }

            refresh();
            xcb_randr_mode_t mode_id = get_id_for_mode(name);
            if (!mode_id) {
                throw std::runtime_error("Failed to find RANDR mode we just created");
            }
            xcb_check(randr_add_output_mode_checked, xcb_randr_get_screen_resources_outputs(res)[0], mode_id);
            self.handle_error("Failed to add RANDR mode we just created to screen");
        }

        void delete_mode(const char* name) {
            xcb_randr_mode_t mode_id = get_id_for_mode(name);
            if (mode_id) {
                xcb_check(randr_delete_output_mode_checked, xcb_randr_get_screen_resources_outputs(res)[0], mode_id);
                self.handle_error("Failed to detach RANDR mode from output");

                xcb_check(randr_destroy_mode_checked, mode_id);
                self.handle_error("Failed to destroy RANDR mode we just detached from output");

                refresh();
            }
        }

        void switch_to_mode(const char *name) {
            xcb_randr_mode_t mode_id = XCB_NONE;
            xcb_randr_output_t* outputs{};
            int noutput = 0;
            if (name) {
                mode_id = get_id_for_mode(name);
                if (mode_id == 0) return;
                outputs = xcb_randr_get_screen_resources_outputs(res);
                noutput = xcb_randr_get_screen_resources_outputs_length(res);
            }

            ALOGE("crts len %d", xcb_randr_get_screen_resources_crtcs_length(res));

            auto reply = xcb(randr_set_crtc_config, xcb_randr_get_screen_resources_crtcs(res)[0], XCB_CURRENT_TIME,
                res->config_timestamp, 0, 0, mode_id, XCB_RANDR_ROTATION_ROTATE_0,
                noutput, outputs);
            self.handle_error(reply,"Failed to switch RANDR mode");
        };

        void set_screen_size(u16 width, u16 height, u32 mm_width, u32 mm_height) {
            auto screen = xcb_setup_roots_iterator(xcb_get_setup (self.conn)).data;
            xcb_check(randr_set_screen_size_checked, screen->root, width, height, mm_width, mm_height);
            self.handle_error("Failed to set RANDR screen size");
        }

    } randr {*this};

    void init(int sockfd) {
        xcb_connection_t* new_conn = xcb_connect_to_fd(sockfd, nullptr);
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

        if (err_ctx)
            xcb_errors_context_free(err_ctx);
        if (conn)
            xcb_disconnect(conn);
        conn = new_conn;
        xcb_errors_context_new(conn, &err_ctx);

        shm.init();
        //randr.init();
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

class lorie_client {
public:
    lorie_message_queue queue;
    std::thread runner_thread;
    bool terminate = false;

    xcb_connection c;

    struct {
        ANativeWindow* win{};
        u32 width{};
        u32 height{};

        i32 shmfd{};
        xcb_shm_seg_t shmseg{};
        u32 *shmaddr{};
    } screen;
    ANativeWindow* cursor{};


    lorie_client() {
        runner_thread = std::thread([=, this] { runner(); });
    }

    void post(std::function<void()> task) {
        queue.write(std::move(task));
    }

    void runner() {
        ALooper_prepare(0);
        ALooper_addFd(ALooper_forThread(), queue.get_fd(), ALOOPER_EVENT_INPUT, ALOOPER_POLL_CALLBACK, [](int, int, void *d) {
            auto thiz = reinterpret_cast<lorie_client*> (d);
            thiz->queue.run();
            return 1;
        }, this);

        while(!terminate) ALooper_pollAll(500, nullptr, nullptr, nullptr);

        ALooper_release(ALooper_forThread());
    }

    void surface_changed(ANativeWindow* win, u32 width, u32 height) {
        if (screen.win)
            ANativeWindow_release(screen.win);

        screen.win = win;
        screen.width = width;
        screen.height = height;

        if (c.conn)
            change_resolution(width, height);
    }

    void change_resolution(u16 width, u16 height) {
        char mode_name[128]{};

        auto mi = libxcvt_gen_mode_info(width, height, 60, false, false);
        ALOGE("Changing resolution to %dx%d", mi->hdisplay, mi->vdisplay);
        sprintf(mode_name, "TERMUX:X11 %dx%d", mi->hdisplay, mi->vdisplay);

        int mm_width = int(25.4*width/120);
        int mm_height = int(25.4*height/120);

        xcb_grab_server(c.conn);
        try {
            ALOGE("line %d", __LINE__);
            c.randr.create_mode(c.randr.temporary_name, mi);
            c.randr.switch_to_mode(nullptr);
            ALOGE("line %d", __LINE__);
            c.randr.set_screen_size(mi->hdisplay, mi->vdisplay, mm_width, mm_height);
            c.randr.switch_to_mode(c.randr.temporary_name);
            ALOGE("line %d", __LINE__);
            c.randr.delete_mode(mode_name);
            c.randr.create_mode(mode_name, mi); // NOLINT(cppcoreguidelines-narrowing-conversions)
            ALOGE("line %d", __LINE__);
            c.randr.switch_to_mode(mode_name);
            c.randr.delete_mode(c.randr.temporary_name);
            ALOGE("line %d", __LINE__);
        } catch (std::runtime_error& e) {
            xcb_ungrab_server(c.conn);
            throw e;
        }
        xcb_ungrab_server(c.conn);
    }

    void adopt_connection_fd(int fd) {
        ALOGE("Connecting to fd %d", fd);
        c.init(fd);
        xcb_screen_t* scr = xcb_setup_roots_iterator(xcb_get_setup(c.conn)).data;

        xcb_change_window_attributes(c.conn, scr->root, XCB_CW_EVENT_MASK, (const int[]) { XCB_EVENT_MASK_STRUCTURE_NOTIFY });
        c.fixes.select_input(scr->root, XCB_XFIXES_CURSOR_NOTIFY_MASK_DISPLAY_CURSOR);
        c.damage.create(scr->root, XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);
        struct {
            xcb_input_event_mask_t head;
            xcb_input_xi_event_mask_t mask;
        } mask{};
        mask.head.deviceid = c.input.client_pointer_id();
        mask.head.mask_len = sizeof(mask.mask) / sizeof(uint32_t);
        mask.mask = XCB_INPUT_XI_EVENT_MASK_RAW_MOTION;
        c.input.select_events(scr->root, 1, &mask.head);

        screen.shmseg = xcb_generate_id(c.conn);

        if (screen.shmaddr)
            munmap(screen.shmaddr, DEFAULT_SHMSEG_LENGTH);
        if (screen.shmfd)
            close(screen.shmfd);

        ALOGE("Creating file...");
        screen.shmfd = os_create_anonymous_file(DEFAULT_SHMSEG_LENGTH);
        if (screen.shmfd < 1) {
            ALOGE("Error opening file: %s", strerror(errno));
        }
        fchmod(screen.shmfd, 0777);
        ALOGE("Attaching file...");
        screen.shmaddr = static_cast<u32 *>(mmap(nullptr, 8096*8096*4,
                                                      PROT_READ | PROT_WRITE,
                                                      MAP_SHARED, screen.shmfd, 0));
        if (screen.shmaddr == MAP_FAILED) {
            ALOGE("Map failed: %s", strerror(errno));
        }
        c.shm.attach_fd(screen.shmseg, screen.shmfd, 0);

        if (screen.win) {
            change_resolution(screen.width, screen.height);
        }

        int event_mask = ALOOPER_EVENT_INPUT | ALOOPER_EVENT_OUTPUT | ALOOPER_EVENT_INVALID | ALOOPER_EVENT_HANGUP | ALOOPER_EVENT_ERROR;
        ALooper_addFd(ALooper_forThread(), fd, ALOOPER_EVENT_INPUT, event_mask, [](int, int mask, void *d) {
            auto self = reinterpret_cast<lorie_client*>(d);
            if (mask & (ALOOPER_EVENT_INVALID | ALOOPER_EVENT_HANGUP | ALOOPER_EVENT_ERROR)) {
                xcb_disconnect(self->c.conn);
                self->c.conn = nullptr;
                ALOGE("Disconnected");
                return 0;
            }

            self->connection_poll_func();
            return 1;
        }, this);
    }

    void connection_poll_func() {
        xcb_generic_event_t *event;
        const char* ext;
        xcb_screen_t* s = xcb_setup_roots_iterator(xcb_get_setup(c.conn)).data;

        while((event = xcb_poll_for_event(c.conn))) {
            if (event->response_type == 0) {
                c.err = reinterpret_cast<xcb_generic_error_t *>(event);
                c.handle_error("Error processing XCB events");
            } else if (event->response_type == XCB_CONFIGURE_NOTIFY) {
                ALOGE("Configure notification. ");
                auto e = reinterpret_cast<xcb_configure_request_event_t *>(event);
                ALOGE("old w: %d h: %d", s->width_in_pixels, s->height_in_pixels);
                ALOGE("new w: %d h: %d", e->width, e->height);
                s->width_in_pixels = e->width;
                s->height_in_pixels = e->height;
            } else if (c.damage.is_damage_notify_event(event)) {
                try {
                    c.shm.get(s->root, 0, 0, s->width_in_pixels, s->height_in_pixels, ~0, // NOLINT(cppcoreguidelines-narrowing-conversions)
                              XCB_IMAGE_FORMAT_Z_PIXMAP, screen.shmseg, 0);

                    blit_exact(screen.win, screen.shmaddr, s->width_in_pixels, s->height_in_pixels);
                } catch (std::runtime_error &err) {
                    continue;
                }
            } //else
            //  ALOGE("some other event %s of %s", xcb_errors_get_name_for_core_event(c.err_ctx, event->response_type, &ext), (ext ?: "core"));
        }
    }

    ~lorie_client() {
        queue.write([=, this] { terminate = true; });
        if (runner_thread.joinable())
            runner_thread.join();
        close(queue.get_fd());
    }
} lorie_client; // NOLINT(cert-err58-cpp)

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_start(unused JNIEnv *env, unused jobject thiz, jint fd) {
    lorie_client.post([fd] { lorie_client.adopt_connection_fd(fd); });
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_surface(JNIEnv *env, jobject thiz, jobject sfc, jint width, jint height) {
    ANativeWindow *win = sfc ? ANativeWindow_fromSurface(env, sfc) : nullptr;
    if (win)
        ANativeWindow_acquire(win);

    ALOGE("Got new surface %p", win);
    lorie_client.post([=] { lorie_client.surface_changed(win, width, height); });
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