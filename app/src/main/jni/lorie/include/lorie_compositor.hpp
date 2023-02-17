#pragma once
#include <lorie_wayland_server.hpp>
#include <wayland-server-protocol.hpp>
#include <xdg-shell-server-protocol.hpp>
#include <lorie_message_queue.hpp>
#include "log.h"

#ifdef ANDROID
#include <android/native_window.h>
#include <jni.h>
#include <thread>

typedef ANativeWindow* EGLNativeWindowType;
#else
typedef void* EGLNativeWindowType;
#endif

class lorie_compositor: public wayland::display_t {
public:
    lorie_compositor();
// compositor features
    void start();
    void post(std::function<void()> f);
    void redraw(bool force = false);

    void output_resize(EGLNativeWindowType win, int real_width, int real_height, int physical_width, int physical_height);
    void report_mode(wayland::output_t* output) const;

    void pointer_motion(int x, int y); // absolute values
    void pointer_scroll(int axis, float value);
    void pointer_button(int button, int state);
    void keyboard_key(uint32_t key, wayland::keyboard_key_state state);

    struct client_data {
        wayland::output_t* output{};
        wayland::pointer_t* pointer{};
        wayland::keyboard_t* kbd{};
        wayland::touch_t* touch{};
    };

    struct surface_data {
        uint32_t x{}, y{};
        bool damaged{};
        wayland::buffer_t *buffer{};
        wayland::callback_t *frame_callback{};
    };

    struct {
        int real_width, real_height;
        int physical_width, physical_height;
        wayland::surface_t* sfc;
        EGLNativeWindowType win;
    } screen{};
    struct {
        int width, height;
        int hotspot_x, hotspot_y;
        int x, y;
        wayland::surface_t* sfc;
        EGLNativeWindowType win;
    } cursor{};
    static void blit(EGLNativeWindowType win, wayland::surface_t* sfc);
    std::function<void(bool)> set_renderer_visibility = [](bool){};
    std::function<void(bool)> set_cursor_visibility = [](bool){};
    std::function<void(int, int)> set_cursor_position = [](int, int){};

// backend features
    void get_keymap(int *fd, int *size);

//private:
    wayland::global_compositor_t global_compositor{this};
    wayland::global_seat_t global_seat{this};
    wayland::global_output_t global_output{this};
    wayland::global_shell_t global_shell{this};
    wayland::global_xdg_wm_base_t global_xdg_wm_base{this};

	lorie_message_queue queue;

#ifdef ANDROID
    JNIEnv *env{};
    jobject thiz{};
    static jfieldID compositor_field_id;
    jmethodID set_renderer_visibility_id{};
    jmethodID set_cursor_visibility_id{};
    jmethodID set_cursor_rect_id{};
    lorie_compositor(jobject thiz);
    std::thread self;
#endif
};
