#include <thread>
#include "lorie_compositor.hpp"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "ashmem.h"
#include <csignal>

#include <android/native_window_jni.h>
#include <sys/socket.h>
#include <dirent.h>
#include <android/log.h>

#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#define unused __attribute__((__unused__))
#define always_inline __attribute__((always_inline)) inline

JavaVM *vm{};

jfieldID lorie_compositor::compositor_field_id{};

lorie_compositor::lorie_compositor(jobject thiz): lorie_compositor() {
    this->thiz = thiz;
	self = std::thread([=, this]{
		vm->AttachCurrentThread(&env, nullptr);
		compositor_field_id = env->GetFieldID(env->GetObjectClass(thiz), "compositor", "J");
		set_renderer_visibility_id = env->GetMethodID(env->GetObjectClass(thiz), "setRendererVisibility", "(Z)V");
		set_cursor_visibility_id = env->GetMethodID(env->GetObjectClass(thiz), "setCursorVisibility", "(Z)V");
		set_cursor_rect_id =env->GetMethodID( env->GetObjectClass(thiz), "setCursorRect", "(IIII)V");

        set_renderer_visibility = [=](bool visible) {
            env->CallVoidMethod(thiz, set_renderer_visibility_id, visible);
        };

        set_cursor_visibility = [=](bool visible) {
            env->CallVoidMethod(thiz, set_cursor_visibility_id, visible);
            if (!visible)
                set_cursor_position = [=](int, int) {};
            else
                set_cursor_position = [=](int x, int y) {
                    auto b = cursor.sfc ? any_cast<surface_data*>(cursor.sfc->user_data())->buffer : nullptr;
                    int sx = x - cursor.hotspot_x;
                    int sy = y - cursor.hotspot_y;
                    int w = b ? b->shm_width() : 0;
                    int h = b ? b->shm_width() : 0;
                    env->CallVoidMethod(thiz, set_cursor_rect_id, sx, sy, w, h);
                };
        };

		run();

		vm->DetachCurrentThread();
		env = nullptr;
	});
}

void lorie_compositor::get_keymap(int *fd, int *size) { // NOLINT(readability-convert-member-functions-to-static)
    int keymap_fd = open("/data/data/com.termux.x11/files/en_us.xkbmap", O_RDONLY);
	struct stat s = {};
	fstat(keymap_fd, &s);
	*size = s.st_size; // NOLINT(cppcoreguidelines-narrowing-conversions)
	*fd = keymap_fd;
}

// For some reason both static_cast and reinterpret_cast returning 0 when casting b.bits.
static always_inline uint32_t* cast(void* p) { union { void* a; uint32_t* b; } c {p}; return c.b; } // NOLINT(cppcoreguidelines-pro-type-member-init)

static always_inline void blit_exact(EGLNativeWindowType win, const uint32_t* src, int width, int height) {
    if (width == 0 || height == 0) {
        width = ANativeWindow_getWidth(win);
        height = ANativeWindow_getHeight(win);
    }
    ARect bounds{ 0, 0, width, height };
    ANativeWindow_Buffer b{};

    ANativeWindow_acquire(win);
    auto ret = ANativeWindow_setBuffersGeometry(win, width, height, AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM);
    if (ret != 0) {
        LOGE("Failed to set buffers geometry (%d)", ret);
        return;
    }

    ret = ANativeWindow_lock(win, &b, &bounds);
    if (ret != 0) {
        LOGE("Failed to lock");
        return;
    }

    uint32_t* dst = cast(b.bits);
    if (src) {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                //uint32_t* d = &dst[b.stride*i + j];
                uint32_t s = src[width * i + j];
                // Cast BGRA to RGBA
                dst[b.stride * i + j] = ((s & 0x0000FF) << 16) | (s & 0x00FF00) | ((s & 0xFF0000) >> 16);
            }
        }
    } else
        memset(dst, 0, b.stride*b.height);

    ret = ANativeWindow_unlockAndPost(win);
    if (ret != 0) {
        LOGE("Failed to post");
        return;
    }

    ANativeWindow_release(win);
}

void lorie_compositor::blit(EGLNativeWindowType win, wayland::surface_t* sfc) {
	if (!win)
        return;

    auto buffer = sfc ? any_cast<surface_data*>(sfc->user_data())->buffer : nullptr;
    if (buffer)
        blit_exact(win, cast(buffer->shm_data()), buffer->shm_width(), buffer->shm_height());
    else
        blit_exact(win, nullptr, 0, 0);
}

template<class F, F f, auto defaultValue = 0> struct wrapper_impl;
template<class R, class C, class... A, R(C::*f)(A...), auto defaultValue>
struct wrapper_impl<R(C::*)(A...), f, defaultValue> {
	// Be careful and do passing jobjects here!!!
	[[maybe_unused]] static always_inline R execute(JNIEnv* env, jobject obj, A... args) {
		auto native = lorie_compositor::compositor_field_id ? reinterpret_cast<C*>(env->GetLongField(obj, lorie_compositor::compositor_field_id)) : nullptr;
		if (native != nullptr)
			return (native->*f)(args...);
		return static_cast<R>(defaultValue);
	}

	[[maybe_unused]] static always_inline void queue(JNIEnv* env, jobject obj, A... args) {
        auto native = lorie_compositor::compositor_field_id ? reinterpret_cast<C*>(env->GetLongField(obj, lorie_compositor::compositor_field_id)) : nullptr;
		if (native != nullptr)
			native->post([=]{ (native->*f)(args...); });
	}
};
template<auto f, auto defaultValue = 0>
auto execute = wrapper_impl<decltype(f), f, defaultValue>::execute;
template<auto f, auto defaultValue = 0>
auto queue = wrapper_impl<decltype(f), f, defaultValue>::queue;

[[maybe_unused]] static always_inline lorie_compositor* get(JNIEnv* env, jobject obj) {
    return lorie_compositor::compositor_field_id ?
        reinterpret_cast<lorie_compositor*>(env->GetLongField(obj, lorie_compositor::compositor_field_id)) :
        nullptr;
}

///////////////////////////////////////////////////////////

#define JNI_DECLARE_INNER(package, classname, methodname ) \
     Java_ ## package ## _ ## classname  ## _ ## methodname
#define JNI_DECLARE(classname, methodname) \
     JNI_DECLARE_INNER(com_termux_x11, classname, methodname)

extern "C" JNIEXPORT jlong JNICALL
JNI_DECLARE(LorieService, createLorieThread)(JNIEnv *env, jobject thiz) {
#if 0
	// It is needed to redirect stderr to logcat
	setenv("WAYLAND_DEBUG", "1", 1);
	new std::thread([]{
		FILE *fp;
		int p[2];
		size_t read, len;
		char* line = nullptr;
		pipe(p);
		fp = fdopen(p[0], "r");

		dup2(p[1], 2);
		while ((read = getline(&line, &len, fp)) != -1) {
			__android_log_write(ANDROID_LOG_VERBOSE, "WAYLAND_STDERR", line);
		}
	});
#endif
    return (jlong) new lorie_compositor(env->NewGlobalRef(thiz));
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, passWaylandFD)(JNIEnv *env, jobject thiz, jint fd) {
    LOGI("JNI: got fd %d", fd);
    execute<&lorie_compositor::add_socket_fd>(env, thiz, fd);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, terminate)(JNIEnv *env, jobject obj) {
    auto b = reinterpret_cast<lorie_compositor*>(env->GetLongField(obj, lorie_compositor::compositor_field_id));
    b->terminate();
    b->self.join();
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, cursorChanged)(JNIEnv *env, jobject thiz, jobject surface) {
	EGLNativeWindowType win = surface?ANativeWindow_fromSurface(env, surface):nullptr;
	auto c = get(env, thiz);
	c->post([=]{ c->cursor.win = win; });
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, windowChanged)(JNIEnv *env, jobject thiz, jobject surface, jint width, jint height, jint mm_width, jint mm_height) {
	EGLNativeWindowType win = surface?ANativeWindow_fromSurface(env, surface):nullptr;
    queue<&lorie_compositor::output_resize>(env, thiz, win, width, height, mm_width, mm_height);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchDown)(JNIEnv *env, jobject thiz, jint id, jfloat x, jfloat y) {
    queue<&lorie_compositor::touch_down>(env, thiz, static_cast<uint32_t>(id), static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchMotion)(JNIEnv *env, jobject thiz, jint id, jfloat x, jfloat y) {
    queue<&lorie_compositor::touch_motion>(env, thiz, static_cast<uint32_t>(id), static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchUp)(JNIEnv *env, jobject thiz, jint id) {
    queue<&lorie_compositor::touch_up>(env, thiz, id);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchFrame)(JNIEnv *env, jobject thiz) {
    queue<&lorie_compositor::touch_frame>(env, thiz);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerMotion)(JNIEnv *env, jobject thiz, jint x, jint y) {
    queue<&lorie_compositor::pointer_motion>(env, thiz, x, y);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerScroll)(JNIEnv *env, jobject thiz, jint axis, jfloat value) {
    queue<&lorie_compositor::pointer_scroll>(env, thiz, axis, value);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerButton)(JNIEnv *env, jobject thiz, jint button, jint type) {
    queue<&lorie_compositor::pointer_button>(env, thiz, uint32_t(button), uint32_t(type));
}

extern "C" void get_character_data(char** layout, int *shift, int *ec, char *ch);
extern "C" void android_keycode_get_eventcode(int kc, int *ec, int *shift);

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, keyboardKey)(JNIEnv *env, jobject thiz,
                                           jint type, jint key_code, jint jshift, jstring characters_) {
    char *characters = nullptr;

	int event_code = 0;
    int shift = jshift;
	if (characters_ != nullptr) characters = (char*) env->GetStringUTFChars(characters_, nullptr);
    if (key_code && !characters) {
		android_keycode_get_eventcode(key_code, &event_code, &shift);
		LOGE("kc: %d ec: %d", key_code, event_code);
    }
    if (!key_code && characters) {
        char *layout = nullptr;
        get_character_data(&layout, &shift, &event_code, characters);
    }
	LOGE("Keyboard input: keyCode: %d; eventCode: %d; characters: %s; shift: %d, type: %d", key_code, event_code, characters, shift, type);

    if (shift || jshift)
        queue<&lorie_compositor::keyboard_key>(env, thiz, 42, wayland::keyboard_key_state::pressed);

    // For some reason Android do not send ACTION_DOWN for non-English characters
    if (characters)
        queue<&lorie_compositor::keyboard_key>(env, thiz, event_code, wayland::keyboard_key_state::pressed);

    queue<&lorie_compositor::keyboard_key>(env, thiz, event_code, wayland::keyboard_key_state(type));

    if (shift || jshift)
        queue<&lorie_compositor::keyboard_key>(env, thiz, 42, wayland::keyboard_key_state::released);

    if (characters_ != nullptr) env->ReleaseStringUTFChars(characters_, characters);
}

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
		LOGE("opendir: %s", strerror(errno));
		return;
	}

	while((dir_elem = readdir(proc)) != nullptr) {
		if (!(pid = (pid_t) atoi (dir_elem->d_name)) || pid == self || !sameUid(pid)) // NOLINT(cert-err34-c)
			continue;

		memset(path, 0, sizeof(path));
		memset(link, 0, sizeof(link));
		sprintf(path, "/proc/%d/exe", pid);
		if (readlink(path, link, sizeof(link)) < 0) {
			LOGE("readlink %s: %s", path, strerror(errno));
			continue;
		}
		if (strstr(link, "/logcat") != nullptr) {
			if (kill(pid, SIGKILL) < 0) {
				LOGE("kill %d (%s): %s", pid, link, strerror);
			}
		}
	}
}

void fork(const std::function<void()>& f) {
	switch(fork()) {
		case -1: LOGE("fork: %s", strerror(errno)); return;
		case 0: f(); return;
		default: return;
	}
}

extern "C" JNIEXPORT void JNICALL
Java_com_termux_x11_LorieService_startLogcatForFd(unused JNIEnv *env, unused jclass clazz, jint fd) {
	killAllLogcats();

	LOGI("Starting logcat with output to given fd");
	fork([]() {
		execl("/system/bin/logcat", "logcat", "-c", nullptr);
		LOGE("exec logcat: %s", strerror(errno));
	});

	fork([fd]() {
		dup2(fd, 1);
		dup2(fd, 2);
		execl("/system/bin/logcat", "logcat", nullptr);
		LOGE("exec logcat: %s", strerror(errno));
	});
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
extern "C" jint JNI_OnLoad(JavaVM* vm, [[maybe_unused]] void* reserved) {
	::vm = vm;
	return JNI_VERSION_1_6;
}
#pragma clang diagnostic pop