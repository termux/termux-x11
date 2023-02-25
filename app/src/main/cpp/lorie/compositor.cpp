#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <jni.h>
#include <thread>
#include <cstring>
#include <cerrno>
#include <mutex>
#include <queue>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <android/native_window_jni.h>
#include <android/looper.h>
#include <android/log.h>

#include <lorie_wayland_server.hpp>
#include <wayland-server-protocol.hpp>
#include <xdg-shell-server-protocol.hpp>

#define DEFAULT_SOCKET_NAME "termux-x11"
#define DEFAULT_SOCKET_PATH "/data/data/com.termux/files/usr/tmp"

#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "LorieNative"
#endif

#ifndef LOG
#define LOG(prio, ...) __android_log_print(prio, LOG_TAG, __VA_ARGS__)
#endif

#define LOGV(...) LOG(ANDROID_LOG_VERBOSE, __VA_ARGS__)
#define LOGE(...) LOG(ANDROID_LOG_ERROR, __VA_ARGS__)

#pragma ide diagnostic ignored "EndlessLoop"
#pragma clang diagnostic ignored "-Wshadow"

class lorie_message_queue {
public:
    lorie_message_queue() {
        std::unique_lock<std::mutex> lock(mutex);

        fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (fd == -1) {
            LOGE("Failed to create socketpair for message queue: %s", strerror(errno));
            return;
        }
    };

    void write(std::function<void()> func) {
        static uint64_t i = 1;
        std::unique_lock<std::mutex> lock(mutex);
        queue.push(std::move(func));
        ::write(fd, &i, sizeof(uint64_t));
    };

    static int run(int, uint32_t, void* data) {
        auto q = reinterpret_cast<lorie_message_queue*>(data);
        static uint64_t i = 0;
        std::unique_lock<std::mutex> lock(q->mutex);
        std::function<void()> fn;
        ::read(q->fd, &i, sizeof(uint64_t));
        while(!q->queue.empty()){
            fn = q->queue.front();
            q->queue.pop();

            lock.unlock();
            fn();
            lock.lock();
        }

        return 0;
    };

    int fd;
private:
    std::mutex mutex;
    std::queue<std::function<void()>> queue;
};

class lorie_compositor: public wayland::display_t {
public:
    explicit lorie_compositor(int dpi);

    void report_mode(int width, int height);

    wayland::global_compositor_t global_compositor{this};
    wayland::global_seat_t global_seat{this};
    wayland::global_output_t global_output{this};
    wayland::global_shell_t global_shell{this};
    wayland::global_xdg_wm_base_t global_xdg_wm_base{this};

    lorie_message_queue queue;

    int dpi;
};

using namespace wayland;

__asm__ (
        "     .global blob\n"
        "     .global blob_size\n"
        "     .section .rodata\n"
        " blob:\n"
        "     .incbin \"en_us.xkbmap\"\n"
        " 1:\n"
        " blob_size:\n"
        "     .int 1b - blob"
        );

extern jbyte blob[];
extern int blob_size;

#include <linux/ioctl.h>
#include <linux/un.h>

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

lorie_compositor::lorie_compositor(int dpi): dpi(dpi) {
    // these calls do no overwrite existing values so it is ok.
    setenv("WAYLAND_DISPLAY", DEFAULT_SOCKET_NAME, 0);
    setenv("XDG_RUNTIME_DIR", DEFAULT_SOCKET_PATH, 0);

    // wl_display_add_socket creates socket with name $WAYLAND_DISPLAY in folder $XDG_RUNTIME_DIR
    wl_display_add_socket(*this, nullptr);

    on_client = [=](client_t* client) {
        client->on_destroy = [=] {};
    };
    global_compositor.on_bind = [=] (client_t*, compositor_t* compositor) {
        compositor->on_create_region = [](region_t* region) { region->on__destroy = [=] { region->destroy(); }; };
        compositor->on_create_surface = [=](surface_t* surface) {
            // We need not to send frame callback. That is a sign for Xwayland to send new buffer
            surface->on_attach = [=](buffer_t* b, int32_t, int32_t) {
                // sending WL_BUFFER_RELEASE
                wl_resource_post_event(*b, 0);
            };
            surface->on__destroy = [=] { surface->destroy(); };
        };
    };
    global_seat.on_bind = [=](client_t* client, seat_t* seat) {
        seat->capabilities (seat_capability::keyboard | seat_capability::pointer);
        seat->name("default");

        seat->on_get_pointer = [](pointer_t* pointer) { pointer->on_release = [=] { pointer->destroy(); }; };
        seat->on_get_keyboard = [=](keyboard_t* kbd) {
            kbd->on_release = [=] { kbd->destroy(); };
            int fd = os_create_anonymous_file(blob_size);
            if (fd == -1) return;
            void *dest = mmap(nullptr, blob_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            memcpy(dest, blob, blob_size);
            munmap(dest, blob_size);

            struct stat s = {};
            fstat(fd, &s);

            kbd->keymap(wayland::keyboard_keymap_format::xkb_v1, fd, s.st_size);
            close (fd);
        };
        seat->on_get_touch = [](touch_t* touch) { touch->on_release = [=] { touch->destroy(); }; };
    };
    global_output.on_bind = [=, this](client_t*, output_t* output) {
        report_mode(800, 600);
        output->on_release = [=]{ output->destroy(); };
    };
    global_shell.on_bind = [](client_t*, shell_t*) {};
    global_xdg_wm_base.on_bind = [](client_t*, xdg_wm_base_t* wm_base) {
        wm_base->on_get_xdg_surface = [](xdg_surface_t*, surface_t*) {};
        wm_base->on__destroy = [=]() { wm_base->destroy(); };
    };

    LOGV("Starting compositor");
    wl_display_init_shm (*this);
    wl_event_loop_add_fd(wl_display_get_event_loop(*this), queue.fd,
                            WL_EVENT_READABLE, &lorie_message_queue::run, &queue);

    // We do not really need to terminate it, user can terminate the whole process.
    std::thread([=]{
        while (true) {
            wl_display_flush_clients(*this);
            wl_event_loop_dispatch(wl_display_get_event_loop(*this), 200);
        }
    }).detach();
}

void lorie_compositor::report_mode(int width, int height) {
    int mm_width  = int(width  * 25.4 / dpi);
    int mm_height = int(height * 25.4 / dpi);
    wl_client* client;

    // We do not store any data of clients so just send it to everybody (but only Xwayland is connected);
    wl_client_for_each(client, wl_display_get_client_list(*this)) {
        auto data = std::make_tuple(width, height, mm_width, mm_height);
        // We do not store pointer to that resource, so iterate
        wl_client_for_each_resource(client, [](wl_resource *r, void *d) {
            auto [width, height, mm_width, mm_height] = *(reinterpret_cast<decltype(data)*>(d));
            const wl_interface *output_interface = &wayland::detail::output_interface;
            if (wl_resource_instance_of(r, output_interface, output_interface)) {
                auto output = reinterpret_cast<output_t *>(resource_t::get(r));
                output->geometry(0, 0, mm_width, mm_height, output_subpixel::unknown, "Lorie", "none", output_transform::normal);
                output->scale(1.0);
                output->mode(output_mode::current | output_mode::preferred, width, height, 60000);
                output->done();
                return WL_ITERATOR_STOP;
            }
            return WL_ITERATOR_CONTINUE;
        }, &data);
    }
}

static lorie_compositor* compositor{};
static int display = 0;

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_CmdEntryPoint_spawnCompositor([[maybe_unused]] JNIEnv *env,
                                                  [[maybe_unused]] jobject thiz,
                                                  jint d, jint dpi) {
    display = d;
    compositor = new lorie_compositor(dpi);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_x11_CmdEntryPoint_outputResize([[maybe_unused]] JNIEnv *env,
                                               [[maybe_unused]] jobject thiz,
                                               jint width, jint height) {
    if (compositor)
        compositor->report_mode(width, height);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_termux_x11_CmdEntryPoint_socketExists([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass clazz) {
    // Here we should try to connect socket...
    struct sockaddr_un local{ .sun_family=AF_UNIX };
    int len, fd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if(fd == -1) {
        perror("Unable to create socket");
        return 1;
    }

    snprintf(local.sun_path, sizeof(local.sun_path), "%s/%s",
            getenv("XDG_RUNTIME_DIR") ?: DEFAULT_SOCKET_PATH,
            getenv("WAYLAND_SOCKET") ?: DEFAULT_SOCKET_NAME);

    len = int(strlen(local.sun_path) + sizeof(local.sun_family));
    return connect(fd, (struct sockaddr *)&local, len) != -1;
}

void exit(JNIEnv* env, int code) {
    // app_process writes exception to logcat if a process dies with exit() or abort(), so
    jclass systemClass = env->FindClass("java/lang/System");
    jmethodID exitMethod = env->GetStaticMethodID(systemClass, "exit", "(I)V");
    env->CallStaticVoidMethod(systemClass, exitMethod, code);
}

extern "C" JNIEXPORT void JNICALL
Java_com_termux_x11_CmdEntryPoint_exec(JNIEnv *env, [[maybe_unused]] jclass clazz, jobjectArray jargv) {
    // execv's argv array is a bit incompatible with Java's String[], so we do some converting here...
    int argc = env->GetArrayLength(jargv) + 2; // Leading executable path and terminating NULL
    char *argv[argc];
    memset(argv, 0, sizeof(char*) * argc);

    argv[0] = (char*) "Xwayland";
    argv[argc] = nullptr; // Terminating NULL
    for(int i=1; i<argc-1; i++) {
        auto js = reinterpret_cast<jstring>(env->GetObjectArrayElement(jargv, i - 1));
        const char *pjc = env->GetStringUTFChars(js, JNI_FALSE);
        argv[i] = static_cast<char *>(calloc(strlen(pjc) + 1, sizeof(char))); //Extra char for the terminating NULL
        strcpy((char *) argv[i], pjc);
        env->ReleaseStringUTFChars(js, pjc);
    }

    // This process should be shut if Xwayland dies...
    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    fcntl(fds[1], F_SETFD, fcntl(fds[1], F_GETFD) | FD_CLOEXEC);
    ALooper_addFd(ALooper_prepare(0), fds[1], ALOOPER_POLL_CALLBACK,
                  ALOOPER_EVENT_INVALID | ALOOPER_EVENT_HANGUP | ALOOPER_EVENT_ERROR,
                  [](int, int, void* data) {
        dprintf(2, "Xwayland died, no need to go on...\n");
        exit(reinterpret_cast<JNIEnv*>(data), 0);
        return 0;
    }, env);

    switch(fork()) {
        case -1:
            perror("fork");
            exit(env, 1);
            break;
        case 0: //child
            execvp(argv[0], argv);
            perror("execvp");
        default: //parent
            close(fds[0]);
            break;
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_termux_x11_CmdEntryPoint_connect([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass clazz) {
    struct sockaddr_un local { .sun_family=AF_UNIX, .sun_path="/data/data/com.termux/files/usr/tmp/.X11-unix/X0" };
    int sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    snprintf(local.sun_path, sizeof(local.sun_path), "%s/.X11-unix/X%d",
             getenv("TMPDIR") ?: DEFAULT_SOCKET_PATH, display);

    if (connect(sockfd, reinterpret_cast<const sockaddr *>(&local), sizeof(local)) < 0) {
        LOGE("Failed to connect %s: %s\n", local.sun_path, strerror(errno));
        return -1;
    }

    return sockfd;
}