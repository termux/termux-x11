#include <sys/ioctl.h>
#include <thread>
#include <cwchar>
#include <vector>
#include <functional>
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
#include <sys/eventfd.h>
#include <sys/vfs.h>
#include <android/native_window_jni.h>
#include <android/looper.h>
#include <bits/epoll_event.h>
#include <sys/epoll.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <dirent.h>

#if 1
#define ALOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "LorieX11Client", fmt, ## __VA_ARGS__)
#define ALOGV(fmt, ...) __android_log_print(ANDROID_LOG_VERBOSE, "LorieX11Client", fmt, ## __VA_ARGS__)
#else
#define ALOGE(fmt, ...) printf(fmt, ## __VA_ARGS__); printf("\n")
#endif

#define always_inline inline __attribute__((__always_inline__))

#include "lorie_message_queue.hpp"
#include "xcb-connection.hpp"
#include "android-keycodes-to-x11-keysyms.h"


// To avoid reopening new segment on every screen resolution
// change we can open it only once with some maximal size
#define DEFAULT_SHMSEG_LENGTH 8192*8192*4

#pragma ide diagnostic ignored "ConstantParameter"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"

static inline int
os_create_anonymous_file(size_t size) {
    int fd, ret;
    long flags;
    fd = open("/dev/ashmem", O_RDWR | O_CLOEXEC);
    if (fd < 0)
        return fd;
    ret = ioctl(fd, /** ASHMEM_SET_SIZE */ _IOW(0x77, 3, size_t), size);
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
    if (!win)
        return;

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
    lorie_looper looper;
    std::thread runner_thread;
    bool terminate = false;
    bool paused = false;

    xcb_connection c;

    struct {
        ANativeWindow* win;
        u32 width, height;

        i32 shmfd;
        xcb_shm_seg_t shmseg;
        u32 *shmaddr;
    } screen {};
    struct {
        ANativeWindow* win;
        u32 width, height, x, y, xhot, yhot;
    } cursor{};


    lorie_client() {
        runner_thread = std::thread([=, this] {
            while(!terminate) {
                looper.dispatch(1000);
            }
        });
    }

    void post(std::function<void()> task) {
        looper.post(std::move(task));
    }

    [[maybe_unused]]
    void post_delayed(std::function<void()> task, long ms_delay) {
        looper.post(std::move(task), ms_delay);
    }

    void surface_changed(ANativeWindow* win, u32 width, u32 height) {
        ALOGE("Surface: changing surface %p to %p", screen.win, win);
        if (screen.win)
            ANativeWindow_release(screen.win);

        if (win)
            ANativeWindow_acquire(win);
        screen.win = win;
        screen.width = width;
        screen.height = height;

        refresh_screen();
    }

    void cursor_changed(ANativeWindow* win) {
        if (cursor.win)
            ANativeWindow_release(cursor.win);

        if (win)
            ANativeWindow_acquire(win);
        cursor.win = win;

        refresh_cursor();
    }

    void attach_region() {
        if (screen.shmaddr)
            munmap(screen.shmaddr, DEFAULT_SHMSEG_LENGTH);
        if (screen.shmfd)
            close(screen.shmfd);

        ALOGE("Creating ashmem file...");
        screen.shmfd = os_create_anonymous_file(DEFAULT_SHMSEG_LENGTH);
        if (screen.shmfd < 0) {
            ALOGE("Error opening file: %s", strerror(errno));
        } else {
            fchmod(screen.shmfd, 0777);
            ALOGE("Attaching file...");
            screen.shmaddr = static_cast<u32 *>(mmap(nullptr, DEFAULT_SHMSEG_LENGTH,
                                                     PROT_READ | PROT_WRITE,
                                                     MAP_SHARED, screen.shmfd, 0));
            if (screen.shmaddr == MAP_FAILED) {
                ALOGE("Map failed: %s", strerror(errno));
            }
        }
        try {
            c.shm.attach_fd(screen.shmseg, screen.shmfd, 0);
            return;
        } catch (std::runtime_error &e) {
            ALOGE("%s", e.what());
            if (screen.shmaddr && screen.shmaddr != MAP_FAILED)
                munmap(screen.shmaddr, DEFAULT_SHMSEG_LENGTH);
            if (screen.shmfd != -1)
                close(screen.shmfd);

            ALOGE("Trying to retrieve shared segment...");
        }

        // There is a chance that Xwayland we are trying to connect is non-termux, try another way
        screen.shmfd = c.shm.create_segment(screen.shmseg, DEFAULT_SHMSEG_LENGTH, 0);
        if (screen.shmfd < 0) {
            ALOGE("Failed to retrieve shared segment file descriptor...");
            return;
        } else {
            struct statfs info{};
            fstatfs(screen.shmfd, &info);
            if (info.f_type != TMPFS_MAGIC) {
                ALOGE("Shared segment is not hosted on tmpfs. You are likely doing something wrong. "
                      "Check if /run/shm, /var/tmp and /tmp are tmpfs.");
                close(screen.shmfd);
                screen.shmfd = -1;
            } else {
                ALOGE("Attaching shared memory segment...");
                screen.shmaddr = static_cast<u32 *>(mmap(nullptr, DEFAULT_SHMSEG_LENGTH,
                                                         PROT_READ | PROT_WRITE,
                                                         MAP_SHARED, screen.shmfd, 0));
                if (screen.shmaddr == MAP_FAILED) {
                    ALOGE("Map failed: %s", strerror(errno));
                }
            }
        }
    }

    void adopt_connection_fd(int fd) {
        try {
            static int old_fd = -1;
            if (old_fd != -1)
                looper.remove_fd(old_fd);

            ALOGE("Connecting to fd %d", fd);
            c.init(fd);
            xcb_screen_t *scr = xcb_setup_roots_iterator(xcb_get_setup(c.conn)).data;

            xcb_change_window_attributes(c.conn, scr->root, XCB_CW_EVENT_MASK,
                                         (const int[]) {XCB_EVENT_MASK_STRUCTURE_NOTIFY});
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

            attach_region();
            refresh_cursor();


            ALOGE("Adding connection with fd %d to poller", fd);
            u32 events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLET | EPOLLHUP | EPOLLNVAL | EPOLLMSG;
            looper.add_fd(fd, events, [&] (u32 mask) {
                    if (mask & (EPOLLNVAL | EPOLLHUP | EPOLLERR)) {
                        looper.remove_fd(old_fd);
                        old_fd = -1;
                        xcb_disconnect(c.conn);
                        c.conn = nullptr;
                        set_renderer_visibility(false);
                        ALOGE("Disconnected");
                        return;
                    }

                    connection_poll_func();
            });
            old_fd = fd;

            set_renderer_visibility(true);
            refresh_screen();
        } catch (std::exception& e) {
            ALOGE("Failed to adopt X connection socket: %s", e.what());
        }
    }

    void refresh_cursor() {
        if (cursor.win && c.conn) {
            if (false)
            {
                xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(c.conn)).data;
                auto reply = xcb_query_pointer_reply(c.conn, xcb_query_pointer(c.conn, s->root),
                                                     nullptr);
                if (reply)
                    set_cursor_coordinates(reply->root_x, reply->root_y);
                free(reply);
            }
            {
                auto reply = c.fixes.get_cursor_image();
                if (reply) {
                    cursor.width = reply->width;
                    cursor.height = reply->height;
                    cursor.xhot = reply->xhot;
                    cursor.yhot = reply->yhot;
                    u32 *image = xcb_xfixes_get_cursor_image_cursor_image(reply);
                    blit_exact(cursor.win, image, reply->width, reply->height);
                    move_cursor_rect(env, cursor.x, cursor.y);
                }
                free(reply);
            }
        }
    }

    void refresh_screen() {
        if (screen.win && c.conn && !paused) {
            try {
                xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(c.conn)).data;
                c.shm.get(s->root, 0, 0, s->width_in_pixels, s->height_in_pixels,
                          ~0, // NOLINT(cppcoreguidelines-narrowing-conversions)
                          XCB_IMAGE_FORMAT_Z_PIXMAP, screen.shmseg, 0);
                blit_exact(screen.win, screen.shmaddr, s->width_in_pixels,
                           s->height_in_pixels);
            } catch (std::runtime_error &err) {
                ALOGE("Refreshing screen failed: %s", err.what());
            }
        }
    }

    void send_keysym(xcb_keysym_t keysym, int meta_state) {
        if (c.conn)
            post([=] {
                c.xkb.send_keysym(keysym, meta_state);
            });
    }

    void send_key(xcb_keysym_t keycode, u8 type) {
        if (c.conn)
            post([=] {
                c.xkb.send_key(keycode, type);
            });
    }

    void send_button(u8 button, u8 type) {
        if (c.conn)
            post([=] {
                c.xkb.send_button(button, type);
            });
    }

    void connection_poll_func() {
        try {
            bool need_redraw = false;
            bool need_reload_keymaps = false;
            xcb_generic_event_t *event;
            // const char *ext;
            xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(c.conn)).data;
            while ((event = xcb_poll_for_event(c.conn))) {
                if (event->response_type == 0) {
                    c.err = reinterpret_cast<xcb_generic_error_t *>(event);
                    c.handle_error("Error processing XCB events");
                } else if (c.xkb.is_xkb_mapping_notify_event(event)) {
                    need_reload_keymaps = true;
                } else if (event->response_type == XCB_CONFIGURE_NOTIFY) {
                    auto e = reinterpret_cast<xcb_configure_request_event_t *>(event);
                    s->width_in_pixels = e->width;
                    s->height_in_pixels = e->height;
                } else if (c.damage.is_damage_notify_event(event)) {
                    need_redraw = true;
                } else if (c.fixes.is_cursor_notify_event(event)) {
                    refresh_cursor();
                }
            }

            if (need_redraw)
                refresh_screen();
            if (need_reload_keymaps)
                c.xkb.reload_keymaps();
            free(event);
        } catch (std::exception& e) {
            ALOGE("Failure during processing X events: %s", e.what());
        }
    }

    ~lorie_client() {
        looper.post([=, this] { terminate = true; });
        if (runner_thread.joinable())
            runner_thread.join();
    }

    JNIEnv* env{};
    jobject thiz{};
    jmethodID set_renderer_visibility_id{};
    jmethodID move_cursor_rect_id{};
    jmethodID set_cursor_coordinates_id{};
    void init_jni(JavaVM* vm, jobject obj) {
        post([=, this] {
            thiz = obj;
            vm->AttachCurrentThread(&env, nullptr);
            set_renderer_visibility_id =
                    env->GetMethodID(env->GetObjectClass(thiz),"setRendererVisibility","(Z)V");
            move_cursor_rect_id =
                    env->GetMethodID(env->GetObjectClass(thiz),"moveCursorRect","(IIII)V");
            set_cursor_coordinates_id =
                    env->GetMethodID(env->GetObjectClass(thiz),"setCursorCoordinates","(II)V");
        });
    }

    void set_renderer_visibility(bool visible) const {
        if (!set_renderer_visibility_id) {
            ALOGE("Something is wrong, `set_renderer_visibility` is null");
            return;
        }

        env->CallVoidMethod(thiz, set_renderer_visibility_id, visible);
    }

    void move_cursor_rect(JNIEnv* e, int x, int y) const {
        if (!move_cursor_rect_id) {
            ALOGE("Something is wrong, `set_renderer_visibility` is null");
            return;
        }

        e->CallVoidMethod(thiz,
                            move_cursor_rect_id,
                            x - cursor.xhot,
                            y - cursor.yhot,
                            cursor.width,
                            cursor.height);
    }

    void set_cursor_coordinates(int x, int y) const {
        if (!set_renderer_visibility_id) {
            ALOGE("Something is wrong, `set_cursor_coordinates_id` is null");
            return;
        }

        env->CallVoidMethod(thiz, set_cursor_coordinates_id, x, y);
    }
} client; // NOLINT(cert-err58-cpp)

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_init(JNIEnv *env, jobject thiz) {
    // Of course I could do that from JNI_OnLoad, but anyway I need to register `thiz` as class instance;
    JavaVM *vm;
    env->GetJavaVM(&vm);
    client.init_jni(vm, env->NewGlobalRef(thiz));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_connect([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz, jint fd) {
    client.post([fd] { client.adopt_connection_fd(fd); });
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_cursorChanged(JNIEnv *env, [[maybe_unused]] jobject thiz, jobject sfc) {
    ANativeWindow *win = sfc ? ANativeWindow_fromSurface(env, sfc) : nullptr;
    if (win)
        ANativeWindow_acquire(win);

    ALOGE("Cursor: got new surface %p", win);
    client.post([=] { client.cursor_changed(win); });
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_windowChanged(JNIEnv *env, [[maybe_unused]] jobject thiz, jobject sfc,
                                               jint width, jint height) {
    ANativeWindow *win = sfc ? ANativeWindow_fromSurface(env, sfc) : nullptr;
    if (win)
        ANativeWindow_acquire(win);

    ALOGE("Surface: got new surface %p", win);
    client.post([=] { client.surface_changed(win, width, height); });
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_onPointerMotion(JNIEnv *env, [[maybe_unused]] jobject thiz, jint x, jint y) {
    client.cursor.x = x;
    client.cursor.y = y;
    client.move_cursor_rect(env, x, y);

    if (client.c.conn) {
        // XCB is thread safe, no need to post this to dispatch thread.
        xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(client.c.conn)).data;
        xcb_test_fake_input(client.c.conn, XCB_MOTION_NOTIFY, false, XCB_CURRENT_TIME, s->root, x,y,  0);
        xcb_flush(client.c.conn);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_onPointerScroll([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz,
                                                 [[maybe_unused]] jint axis, [[maybe_unused]] jfloat value) {
    // TODO: implement onPointerScroll()
    if (client.c.conn) {
        bool press_shift = axis == 1; // AXIS_X
        int button = value < 0 ? XCB_BUTTON_INDEX_4 : XCB_BUTTON_INDEX_5;
        if (press_shift)
            client.send_key(KEY_LEFTSHIFT, XCB_KEY_PRESS);
        client.send_button(button, XCB_BUTTON_PRESS);
        client.send_button(button, XCB_BUTTON_RELEASE);
        if (press_shift)
            client.send_key(KEY_LEFTSHIFT, XCB_KEY_RELEASE);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_onPointerButton([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz, jint button,
                                                 jint type) {
    client.send_button(button, type);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_onKeySym([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz,
                                               jint keycode, jint unicode, jint meta_state) {
    if (unicode && !meta_state) {
        client.send_keysym(xkb_utf32_to_keysym(unicode), meta_state);
        return;
    }

    if (android_to_keysyms[keycode]) {
        client.send_keysym(android_to_keysyms[keycode], meta_state);
        return;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_onKey([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz, jint keycode, jint type) {
    client.send_key(android_to_linux_keycode[keycode], type);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_nativeResume([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz) {
    client.post([] {
        client.paused = false;
        client.refresh_screen();
    });
}
extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_nativePause([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz) {
    client.paused = true;
}

// I need this to catch initialisation errors of libxkbcommon.
#if 1
// It is needed to redirect stderr to logcat
[[maybe_unused]] std::thread stderr_to_logcat_thread([]{ // NOLINT(cert-err58-cpp)
    FILE *fp;
    int p[2];
    size_t len;
    char* line = nullptr;
    pipe(p);
    fp = fdopen(p[0], "r");

    dup2(p[1], 2);
    dup2(p[1], 1);
    while ((getline(&line, &len, fp)) != -1) {
        __android_log_print(ANDROID_LOG_VERBOSE, "stderr", "%s%s", line, (line[len-1] == '\n') ? "" : "\n");
    }
});
#endif

static bool sameUid(int pid) {
    char path[32] = {0};
    struct stat s = {0};
    sprintf(path, "/proc/%d", pid);
    stat(path, &s);
    return s.st_uid == getuid();
}

static void killAllLogcats() {
    DIR* proc;
    struct dirent* dir_elem;
    char path[64] = {0}, link[64] = {0};
    pid_t pid, self = getpid();
    if ((proc = opendir("/proc")) == nullptr) {
        ALOGE("opendir: %s", strerror(errno));
        return;
    }

    while((dir_elem = readdir(proc)) != nullptr) {
        if (!(pid = (pid_t) atoi (dir_elem->d_name)) || pid == self || !sameUid(pid)) // NOLINT(cert-err34-c)
            continue;

        memset(path, 0, sizeof(path));
        memset(link, 0, sizeof(link));
        sprintf(path, "/proc/%d/exe", pid);
        if (readlink(path, link, sizeof(link)) < 0) {
            ALOGE("readlink %s: %s", path, strerror(errno));
            continue;
        }
        if (strstr(link, "/logcat") != nullptr) {
            if (kill(pid, SIGKILL) < 0) {
                ALOGE("kill %d (%s): %s", pid, link, strerror(errno));
            }
        }
    }
}

void fork(const std::function<void()>& f) {
    switch(fork()) {
        case -1: ALOGE("fork: %s", strerror(errno)); return;
        case 0: f(); return;
        default: return;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_MainActivity_startLogcat([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz, jint fd) {
    killAllLogcats();

    ALOGV("Starting logcat with output to given fd");
    fork([]() {
        execl("/system/bin/logcat", "logcat", "-c", nullptr);
        ALOGE("exec logcat: %s", strerror(errno));
    });

    fork([fd]() {
        dup2(fd, 1);
        dup2(fd, 2);
        execl("/system/bin/logcat", "logcat", nullptr);
        ALOGE("exec logcat: %s", strerror(errno));
    });
}