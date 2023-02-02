#include <thread>
#include <lorie_compositor.hpp>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ashmem.h>
#include <csignal>

#include <lorie_egl_helper.hpp>
#include <android/native_window_jni.h>
#include <sys/socket.h>
#include <dirent.h>
#include <android/log.h>

#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#define unused __attribute__((__unused__))

class lorie_backend_android : public lorie_compositor {
public:
	lorie_backend_android(JavaVM *env, jobject obj, jmethodID pId);

	void backend_init() override;

	void swap_buffers() override;

	void get_keymap(int *fd, int *size) override;

	void window_change_callback(EGLNativeWindowType win, uint32_t width, uint32_t height,
								uint32_t physical_width, uint32_t physical_height);

	void passfd(int fd);

	void on_egl_init();

	LorieEGLHelper helper;

	int keymap_fd = -1;
	std::thread self;

	JavaVM *vm{};
	JNIEnv *env{};
	jobject thiz{};
	static jfieldID compositor_field_id;
	jmethodID set_renderer_visibility_id{};
};

jfieldID lorie_backend_android::compositor_field_id{};

lorie_backend_android::lorie_backend_android(JavaVM *vm, jobject obj, jmethodID mid)
    : vm(vm), thiz(obj), set_renderer_visibility_id(mid), self(&lorie_compositor::start, this) {}

void lorie_backend_android::on_egl_init() {
	renderer.on_surface_create();
}

void lorie_backend_android::backend_init() {
	if (!helper.init(EGL_DEFAULT_DISPLAY)) {
	    LOGE("Failed to initialize EGL context");
	}

	helper.onInit = [this](){ on_egl_init(); };

	renderer.set_renderer_visibility = [=](bool visible) {
		vm->AttachCurrentThread(&env, nullptr);
		env->CallVoidMethod(thiz, set_renderer_visibility_id, visible);
		vm->DetachCurrentThread();
	};
}

void lorie_backend_android::swap_buffers () {
	helper.swap();
}

void lorie_backend_android::get_keymap(int *fd, int *size) {
    if (keymap_fd != -1)
        close(keymap_fd);

    keymap_fd = open("/data/data/com.termux.x11/files/en_us.xkbmap", O_RDONLY);
	struct stat s = {};
	fstat(keymap_fd, &s);
	*size = s.st_size;
	*fd = keymap_fd;
}

void lorie_backend_android::window_change_callback(EGLNativeWindowType win, uint32_t width, uint32_t height, uint32_t physical_width, uint32_t physical_height) {
	LOGV("JNI: window is changed: %p %dx%d (%dmm x %dmm)", win, width, height, physical_width, physical_height);
	helper.setWindow(win);
	post([=, this]() {
        output_resize(width, height, physical_width, physical_height);
	});
}

void lorie_backend_android::passfd(int fd) {
    LOGI("JNI: got fd %d", fd);
    dpy.add_socket_fd(fd);
}

template<class F, F f, auto defaultValue = 0> struct wrapper_impl;
template<class R, class C, class... A, R(C::*f)(A...), auto defaultValue>
struct wrapper_impl<R(C::*)(A...), f, defaultValue> {
	// Be careful and do passing jobjects here!!!
	static inline R execute(JNIEnv* env, jobject obj, A... args) {
		auto native = reinterpret_cast<C*>(env->GetLongField(obj, lorie_backend_android::compositor_field_id));
		if (native != nullptr)
			return (native->*f)(args...);
		return static_cast<R>(defaultValue);
	}
	static inline void queue(JNIEnv* env, jobject obj, A... args) {
		auto native = reinterpret_cast<C*>(env->GetLongField(obj, lorie_backend_android::compositor_field_id));
		if (native != nullptr)
			native->post([=]{ (native->*f)(args...); });
	}
};
template<auto f, auto defaultValue = 0>
auto execute = wrapper_impl<decltype(f), f, defaultValue>::execute;
template<auto f, auto defaultValue = 0>
auto queue = wrapper_impl<decltype(f), f, defaultValue>::queue;

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
	JavaVM* vm{};
	env->GetJavaVM(&vm);
	lorie_backend_android::compositor_field_id = env->GetFieldID(env->GetObjectClass(thiz), "compositor", "J");
	jmethodID set_renderer_visibility_id = env->GetMethodID(env->GetObjectClass(thiz), "setRendererVisibility", "(Z)V");
    return (jlong) new lorie_backend_android(vm, env->NewGlobalRef(thiz),
											 set_renderer_visibility_id);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, passWaylandFD)(JNIEnv *env, jobject thiz, jint fd) {
    execute<&lorie_backend_android::passfd>(env, thiz, fd);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, terminate)(JNIEnv *env, jobject obj) {
    auto b = reinterpret_cast<lorie_backend_android*>(env->GetLongField(obj, lorie_backend_android::compositor_field_id));
    b->terminate();
    b->self.join();
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, windowChanged)(JNIEnv *env, jobject thiz, jobject jsurface, jint width, jint height, jint mm_width, jint mm_height) {
	EGLNativeWindowType win = ANativeWindow_fromSurface(env, jsurface);
    queue<&lorie_backend_android::window_change_callback>(env, thiz, win, width, height, mm_width, mm_height);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchDown)(JNIEnv *env, jobject thiz, jint id, jfloat x, jfloat y) {
    queue<&lorie_backend_android::touch_down>(env, thiz, static_cast<uint32_t>(id), static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchMotion)(JNIEnv *env, jobject thiz, jint id, jfloat x, jfloat y) {
    queue<&lorie_backend_android::touch_motion>(env, thiz, static_cast<uint32_t>(id), static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchUp)(JNIEnv *env, jobject thiz, jint id) {
    queue<&lorie_backend_android::touch_up>(env, thiz, id);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchFrame)(JNIEnv *env, jobject thiz) {
    queue<&lorie_backend_android::touch_frame>(env, thiz);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerMotion)(JNIEnv *env, jobject thiz, jint x, jint y) {
    queue<&lorie_backend_android::pointer_motion>(env, thiz, static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerScroll)(JNIEnv *env, jobject thiz, jint axis, jfloat value) {
    queue<&lorie_backend_android::pointer_scroll>(env, thiz, static_cast<uint32_t>(axis), value);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerButton)(JNIEnv *env, jobject thiz, jint button, jint type) {
    queue<&lorie_backend_android::pointer_button>(env, thiz, static_cast<uint32_t>(button), static_cast<uint32_t>(type));
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
        queue<&lorie_backend_android::keyboard_key>(env, thiz, 42, wayland::keyboard_key_state::pressed);

    // For some reason Android do not send ACTION_DOWN for non-English characters
    if (characters)
        queue<&lorie_backend_android::keyboard_key>(env, thiz, event_code, wayland::keyboard_key_state::pressed);

    queue<&lorie_backend_android::keyboard_key>(env, thiz, event_code, wayland::keyboard_key_state(type));

    if (shift || jshift)
        queue<&lorie_backend_android::keyboard_key>(env, thiz, 42, wayland::keyboard_key_state::released);

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
