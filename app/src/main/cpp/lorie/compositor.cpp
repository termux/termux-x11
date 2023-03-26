#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <android/log.h>
#include <jni.h>

#include <wayland-server.h>

#include <linux/ioctl.h>
#include <linux/un.h>
#include <cassert>

#pragma ide diagnostic ignored "modernize-use-nullptr"
#pragma ide diagnostic ignored "cert-err58-cpp"
#pragma ide diagnostic ignored "ConstantParameter"
#pragma ide diagnostic ignored "EndlessLoop"
#pragma ide diagnostic ignored "NullDereference"
#pragma ide diagnostic ignored "ConstantFunctionResult"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#define DEFAULT_SOCKET_NAME ((char*) "termux-x11")
#define DEFAULT_SOCKET_PATH "/data/data/com.termux/files/usr/tmp"

#include <android/log.h>

#ifndef LOGV
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "LorieNative", __VA_ARGS__)
#endif

#ifndef LOGE
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "LorieNative", __VA_ARGS__)
#endif

class lorie_compositor {
public:
    explicit lorie_compositor(int dpi);
    void report_mode(int width, int height, int dpi);

    wl_display *dpy{};

    int dpi;
};

static lorie_compositor* compositor{};
static int display = 0;


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
#include <cassert>

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

struct client_data {
    wl_resource *touch{};
    wl_resource *pointer{};
    wl_resource *output{};
    wl_resource *shell{};
} *client_data{};

static inline uint32_t serial(wl_display* dpy) {
    return wl_display_next_serial(dpy);
}

static inline uint32_t time() {
    timespec t = {0};
    clock_gettime (CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

int noop_dispatcher(const void*, void *, uint32_t, const wl_message*, wl_argument*) {
    return 0;
}

void compositor_bind(wl_client *client, void*, uint32_t version, uint32_t id) {
    auto comp = wl_resource_create(client, &wl_compositor_interface, std::min(int(version), wl_compositor_interface.version), id);
    wl_resource_set_dispatcher(comp, [](const void*, void *target, uint32_t opcode, const wl_message*, wl_argument *args) {
        auto obj = (wl_resource *) target;
        switch(opcode) { // NOLINT(hicpp-multiway-paths-covered)
            case 0: { // create_surface
                auto surface = wl_resource_create(wl_resource_get_client(obj), &wl_surface_interface, wl_surface_interface.version, args[0].n);
                wl_resource_set_dispatcher(surface, [] (const void*, void *target, uint32_t opcode, const wl_message*, wl_argument *args) {
                    if (opcode == 0) // destroy
                        wl_resource_destroy((wl_resource*) target);

                    // We should send frame callback to signal Xwayland that we finished working with buffer.
                    // We should avoid this because we do not process buffers. Otherways Xwayland will simply send new buffer.
                    if (opcode == 1 && args[0].o) // attach
                        wl_buffer_send_release((wl_resource *) args[0].o);

                    return 0;
                }, &wl_surface_interface, (void *) &wl_surface_interface, nullptr);
                break;
            }
            case 1: { // create_region
                auto region = wl_resource_create(wl_resource_get_client(obj), &wl_region_interface, wl_region_interface.version, args[0].n);
                wl_resource_set_dispatcher(region, [] (const void*, void *target, uint32_t opcode, const wl_message*, wl_argument*) {
                    if (opcode == 0) // destroy
                        wl_resource_destroy((wl_resource*) target);
                    return 0;
                }, &wl_region_interface, (void *) &wl_region_interface, nullptr);
                break;
            }
        }
        return 0;
    }, &wl_compositor_interface, (void*) &wl_compositor_interface, nullptr);
}

void seat_bind(wl_client *client, void*, uint32_t version, uint32_t id) {
    auto seat = wl_resource_create(client, &wl_seat_interface, std::min(int(version), wl_seat_interface.version), id);
    wl_resource_set_dispatcher(seat, [](const void*, void *target, uint32_t opcode, const wl_message*, wl_argument *args) {
        auto obj = (wl_resource *) target;
        auto client = wl_resource_get_client(obj);
        switch(opcode) { // NOLINT(hicpp-multiway-paths-covered)
            case 0: { // get_pointer
                client_data->pointer = wl_resource_create(client, &wl_pointer_interface, wl_pointer_interface.version, args[0].n);
                wl_resource_set_dispatcher(client_data->pointer, noop_dispatcher, &wl_pointer_interface, (void*) &wl_pointer_interface, nullptr);
                break;
            }
            case 1: { // get_keyboard
                auto keyboard = wl_resource_create(client, &wl_keyboard_interface, wl_keyboard_interface.version, args[0].n);
                wl_resource_set_dispatcher(keyboard, noop_dispatcher, &wl_keyboard_interface, (void*) &wl_keyboard_interface, nullptr);

                int fd = os_create_anonymous_file(blob_size);
                if (fd == -1) return 0;
                void *dest = mmap(nullptr, blob_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                memcpy(dest, blob, blob_size);
                munmap(dest, blob_size);

                struct stat s = {};
                fstat(fd, &s);

                wl_keyboard_send_keymap(keyboard, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd, s.st_size);
                close (fd);
                break;
            }
            case 2: { // get_touch
                client_data->touch = wl_resource_create(client, &wl_touch_interface, wl_touch_interface.version, args[0].n);
                wl_resource_set_dispatcher(client_data->touch, noop_dispatcher, &wl_touch_interface, (void*) &wl_touch_interface, nullptr);
                break;
            }
        }
        return 0;
    }, &wl_seat_interface, (void*) &wl_seat_interface, nullptr);

    wl_seat_send_capabilities(seat, WL_SEAT_CAPABILITY_KEYBOARD | WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_TOUCH);
    wl_seat_send_name(seat, "default");
}

void output_bind(wl_client *client, void*, uint32_t version, uint32_t id) {
    client_data->output = wl_resource_create(client, &wl_output_interface, std::min(int(version), wl_output_interface.version), id);
    wl_resource_set_dispatcher(client_data->output, noop_dispatcher, &wl_compositor_interface, (void*) &wl_compositor_interface, nullptr);

    compositor->report_mode(1280, 1024, compositor->dpi);
}

void shell_bind(wl_client *client, void*, uint32_t version, uint32_t id) {
    auto shell = wl_resource_create(client, &wl_shell_interface, std::min(int(version), wl_shell_interface.version), id);
    wl_resource_set_dispatcher(shell, [](const void*, void *target, uint32_t opcode, const wl_message*, wl_argument *args) {
        auto obj = (wl_resource *) target;
        if (opcode == 0) { // get_shell_surface
            auto shell_surface = wl_resource_create(wl_resource_get_client(obj), &wl_shell_surface_interface, wl_shell_surface_interface.version, args[0].n);
            wl_resource_set_dispatcher(shell_surface, noop_dispatcher, &wl_shell_surface_interface, (void *) &wl_shell_surface_interface, nullptr);
            client_data->shell = (wl_resource*) args[1].o;
            if (client_data->pointer && client_data->shell)
                wl_pointer_send_enter(client_data->pointer,
                                      serial(compositor->dpy),
                                      client_data->shell,
                                      wl_fixed_from_int(0),
                                      wl_fixed_from_int(0));
        }
        return 0;
    }, &wl_shell_interface, (void*) &wl_shell_interface, nullptr);
}

const wl_interface xdg_wm_base_interface = {
        "xdg_wm_base", 5,
        4, new const wl_message[] {
                {"destroy", "", new const wl_interface*[] {}},
                {"create_positioner", "n", new const wl_interface*[] {0}},
                {"get_xdg_surface", "no", new const wl_interface*[] {0, 0}},
                {"pong", "u", new const wl_interface*[] {0}},
        },
        1, new const wl_message[] {
                {"ping", "u", new const wl_interface*[] {0}},
        }
};
const wl_interface xdg_toplevel_interface = {
        "xdg_toplevel", 5,
        14, new const wl_message[] {
                {"destroy", "", new const wl_interface*[] {}},
                {"set_parent", "?o", new const wl_interface*[] {&xdg_toplevel_interface}},
                {"set_title", "s", new const wl_interface*[] {0}},
                {"set_app_id", "s", new const wl_interface*[] {0}},
                {"show_window_menu", "ouii", new const wl_interface*[] {&wl_seat_interface, 0, 0, 0}},
                {"move", "ou", new const wl_interface*[] {&wl_seat_interface, 0}},
                {"resize", "ouu", new const wl_interface*[] {&wl_seat_interface, 0, 0}},
                {"set_max_size", "ii", new const wl_interface*[] {0, 0}},
                {"set_min_size", "ii", new const wl_interface*[] {0, 0}},
                {"set_maximized", "", new const wl_interface*[] {}},
                {"unset_maximized", "", new const wl_interface*[] {}},
                {"set_fullscreen", "?o", new const wl_interface*[] {&wl_output_interface}},
                {"unset_fullscreen", "", new const wl_interface*[] {} },
                {"set_minimized", "", new const wl_interface*[] {}},
        },
        4, new const wl_message[] {
                { "configure", "iia", new const wl_interface*[] {0, 0, 0}},
                { "close", "", new const wl_interface*[] {}},
                { "configure_bounds", "4ii", new const wl_interface*[] {0, 0}},
                { "wm_capabilities", "5a", new const wl_interface*[] {0}},
        }
};
const wl_interface xdg_surface_interface = {
        "xdg_surface", 5,
        5, new const wl_message[] {
                {"destroy", "", new const wl_interface*[] {}},
                {"get_toplevel", "n", new const wl_interface*[] {0}},
                {"get_popup", "n?oo", new const wl_interface*[] {0, 0, 0}},
                {"set_window_geometry", "iiii", new const wl_interface*[] {0, 0, 0, 0}},
                {"ack_configure", "u", new const wl_interface*[] {0}},
        },
        1, new const wl_message[] {
                {"configure", "u", new const wl_interface*[] { nullptr,}},
        }
};

void wm_base_bind(wl_client *client, void*, uint32_t version, uint32_t id) {
    auto wm_base = wl_resource_create(client, &xdg_wm_base_interface, std::min(int(version), xdg_wm_base_interface.version), id);
    wl_resource_set_dispatcher(wm_base, [](const void*, void *target, uint32_t opcode, const wl_message*, wl_argument *args) {
        auto obj = (wl_resource *) target;
        if (opcode == 2) { // get_xdg_surface
            auto xdg_surface = wl_resource_create(wl_resource_get_client(obj), &xdg_surface_interface, xdg_surface_interface.version, args[0].n);
            wl_resource_set_dispatcher(xdg_surface, [](const void*, void *target, uint32_t opcode, const wl_message*, wl_argument *args) {
                auto obj = (wl_resource *) target;
                if (opcode == 1) { // get_toplevel
                    auto toplevel = wl_resource_create(wl_resource_get_client(obj), &xdg_toplevel_interface, xdg_wm_base_interface.version, args[0].n);
                    wl_resource_set_dispatcher(toplevel, noop_dispatcher, &xdg_toplevel_interface,(void *) &xdg_toplevel_interface, nullptr);
                }

                return 0;
            }, &xdg_surface_interface, (void *) &xdg_surface_interface, nullptr);
            client_data->shell = (wl_resource*) args[1].o;
            if (client_data->pointer && client_data->shell)
                wl_pointer_send_enter(client_data->pointer,
                                      serial(compositor->dpy),
                                      client_data->shell,
                                      wl_fixed_from_int(0),
                                      wl_fixed_from_int(0));
        }
        return 0;
    }, &xdg_wm_base_interface, (void*) &xdg_wm_base_interface, nullptr);
}

wl_listener client_destroyed_listener = {{}, [](struct wl_listener *, void *data) {
    delete client_data;
    client_data = nullptr;
}};

wl_listener client_listener = {{}, [](struct wl_listener *, void *data) {
    auto client = (wl_client*) data;
    if (client_data != nullptr) {
        pid_t pid;
        wl_client_get_credentials(client, &pid, nullptr, nullptr);
        wl_client_post_implementation_error(client, "You are allowed to connect only one Xwayland client, "
                                                    "and there is already a connected Xwayland client with PID %d", pid);
    } else wl_client_add_destroy_listener(client, &client_destroyed_listener);

    client_data = new struct client_data;
}};

lorie_compositor::lorie_compositor(int dpi): dpi(dpi) {
    // these calls do no overwrite existing values so it is ok.
    setenv("WAYLAND_DISPLAY", DEFAULT_SOCKET_NAME, 0);
    setenv("XDG_RUNTIME_DIR", DEFAULT_SOCKET_PATH, 0);
    LOGV("Starting compositor");
    dpy = wl_display_create();
    // wl_display_add_socket creates socket with name $WAYLAND_DISPLAY in folder $XDG_RUNTIME_DIR
    wl_display_add_socket(dpy, nullptr);
    wl_display_add_client_created_listener(dpy, &client_listener);
    wl_global_create(dpy, &wl_compositor_interface, wl_compositor_interface.version, 0, compositor_bind);
    wl_global_create(dpy, &wl_seat_interface, wl_seat_interface.version, 0, seat_bind);
    wl_global_create(dpy, &wl_output_interface, wl_output_interface.version, 0, output_bind);
    wl_global_create(dpy, &wl_shell_interface, wl_shell_interface.version, 0, shell_bind);
    wl_global_create(dpy, &xdg_wm_base_interface, xdg_wm_base_interface.version, 0, wm_base_bind);

    wl_display_init_shm (dpy);

    // We do not really need to terminate it, user can terminate the whole process.
    std::thread([=]{
        while (true) wl_display_run(dpy);
    }).detach();
}

void lorie_compositor::report_mode(int width, int height, int dpi) { // NOLINT(readability-convert-member-functions-to-static)
    if (width == 0 || height == 0)
        return;
    if (client_data && client_data->output) {
        int mm_width  = int(width  * 25.4 / dpi);
        int mm_height = int(height * 25.4 / dpi);
        wl_output_send_geometry(client_data->output, 0, 0, mm_width, mm_height, 0, "Lorie", "none", 0);
        wl_output_send_scale(client_data->output, 1.0);
        wl_output_send_mode(client_data->output, 0x3, width, height, 60000);
        wl_output_send_done(client_data->output);
        wl_client_flush(wl_resource_get_client(client_data->output));
    }
}

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
        compositor->report_mode(width, height, compositor->dpi);
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
    assert(compositor != nullptr);

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
    wl_event_loop_add_fd(wl_display_get_event_loop(compositor->dpy), fds[1],
                         WL_EVENT_HANGUP | WL_EVENT_ERROR, [](int fd, uint32_t, void* env) {
        dprintf(2, "Xwayland died, no need to go on...\n");
        close(fd);
        ((JNIEnv*)env)->FatalError("Xwayland died, no need to go on...");
        return 0;
    }, env);

    switch(fork()) {
        case -1:
            perror("fork");
            exit(env, 1);
            break;
        case 0: //child
            execvp(argv[0], argv);
            perror("Error starting Xwayland (execvp): ");
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

extern "C"
JNIEXPORT jint JNICALL
Java_com_termux_x11_CmdEntryPoint_stderr([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass clazz) {
    const char *debug = getenv("TERMUX_X11_DEBUG");
    if (debug && !strcmp(debug, "1")) {
        int p[2];
        pipe(p);
        fchmod(p[1], 0777);
        std::thread([=] {
            char buffer[4096];
            int len;
            while((len = read(p[0], buffer, 4096)) >=0)
                write(2, buffer, len);
            close(p[0]);
        }).detach();
        return p[1];
    }

    return -1;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_termux_x11_CmdEntryPoint_getuid([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass clazz) {
    return getuid(); // NOLINT(cppcoreguidelines-narrowing-conversions)
}
