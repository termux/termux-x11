#include <thread>
#include <lorie-compositor.hpp>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ashmem.h>
#include <csignal>

#include <lorie-egl-helper.hpp>
#include <android/native_window_jni.h>
#include <sys/socket.h>
#include <dirent.h>
#include <android/log.h>

#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#define unused __attribute__((__unused__))

#define DEFAULT_WIDTH 480
#define DEFAULT_HEIGHT 800

class LorieBackendAndroid : public LorieCompositor {
public:
	LorieBackendAndroid();

	void backend_init() override;
	uint32_t input_capabilities() override;
	void swap_buffers() override;
	void get_default_proportions(int32_t* width, int32_t* height) override;
	void get_keymap(int *fd, int *size) override;
	void window_change_callback(EGLNativeWindowType win, uint32_t width, uint32_t height, uint32_t physical_width, uint32_t physical_height);
	void layout_change_callback(const char *layout);
	void passfd(int fd);

	void on_egl_init();

	void unused on_egl_uninit();

	LorieEGLHelper helper;

	int keymap_fd = -1;
    std::thread self;
};

LorieBackendAndroid::LorieBackendAndroid()
    : self(&LorieCompositor::start, this) {
}

void LorieBackendAndroid::on_egl_init() {
	renderer.init();
}

void unused LorieBackendAndroid::on_egl_uninit() {
	renderer.uninit();
}

void LorieBackendAndroid::backend_init() {
	if (!helper.init(EGL_DEFAULT_DISPLAY)) {
	    LOGE("Failed to initialize EGL context");
	}

	helper.onInit = [this](){ on_egl_init(); };

}

uint32_t LorieBackendAndroid::input_capabilities() {
	return	WL_SEAT_CAPABILITY_TOUCH |
			WL_SEAT_CAPABILITY_POINTER |
			WL_SEAT_CAPABILITY_KEYBOARD;
}

void LorieBackendAndroid::swap_buffers () {
	helper.swap();
}

void LorieBackendAndroid::get_default_proportions(int32_t* width, int32_t* height) {
	if (width) *width = DEFAULT_WIDTH;
	if (height) *height = DEFAULT_HEIGHT;
}

static int
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

void LorieBackendAndroid::get_keymap(int *fd, int *size) {
    if (keymap_fd != -1)
        close(keymap_fd);

    keymap_fd = open("/data/data/com.termux.x11/files/en_us.xkbmap", O_RDONLY);
	struct stat s = {};
	fstat(keymap_fd, &s);
	*size = s.st_size;
	*fd = keymap_fd;
}

void LorieBackendAndroid::window_change_callback(EGLNativeWindowType win, uint32_t width, uint32_t height, uint32_t physical_width, uint32_t physical_height) {
	LOGE("WindowChangeCallback");
	helper.setWindow(win);
	post([this, width, height, physical_width, physical_height]() {
        output_resize(width, height, physical_width, physical_height);
        });
}

void LorieBackendAndroid::layout_change_callback(const char *layout) {
	post([this]() {
        keyboard_keymap_changed();
    });
}
void LorieBackendAndroid::passfd(int fd) {
    listen(fd, 128);
    wl_display_add_socket_fd(display, fd);
}

///////////////////////////////////////////////////////////

#define JNI_DECLARE_INNER(package, classname, methodname ) \
     Java_ ## package ## _ ## classname  ## _ ## methodname
#define JNI_DECLARE(classname, methodname) \
     JNI_DECLARE_INNER(com_termux_x11, classname, methodname)

#define WL_POINTER_MOTION 2

static LorieBackendAndroid* fromLong(jlong v) {
    return reinterpret_cast<LorieBackendAndroid*>(v);
}

extern "C" JNIEXPORT jlong JNICALL
JNI_DECLARE(LorieService, createLorieThread)(unused JNIEnv *env, unused jobject instance) {
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
    return (jlong) new LorieBackendAndroid;
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, passWaylandFD)(unused JNIEnv *env, unused jobject instance, jlong jcompositor, jint fd) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);
    char path[256] = {0};
    char realpath[256] = {0};
    sprintf(path, "/proc/self/fd/%d", fd);
    readlink(path, realpath, sizeof(realpath)); // NOLINT(bugprone-unused-return-value)
    LOGI("JNI: got fd %d (%s)", fd, realpath);

    b->passfd(fd);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, terminate)(unused JNIEnv *env, unused jobject instance,  jlong jcompositor) {
    if (jcompositor == 0) return;
        LorieBackendAndroid *b = fromLong(jcompositor);
    LOGI("JNI: requested termination");
    b->post([b](){
        b->terminate();
    });
    b->self.join();
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, windowChanged)(JNIEnv *env, unused jobject instance, jlong jcompositor, jobject jsurface, jint width, jint height, jint mmWidth, jint mmHeight) {
	if (jcompositor == 0) return;
	LorieBackendAndroid *b = fromLong(jcompositor);

	EGLNativeWindowType win = ANativeWindow_fromSurface(env, jsurface);
	b->post([b, win, width, height, mmWidth, mmHeight](){
        b->window_change_callback(win, width, height, mmWidth, mmHeight);
	});

    LOGV("JNI: window is changed: %p(%p) %dx%d (%dmm x %dmm)", win, jsurface, width, height, mmWidth, mmHeight);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchDown)(unused JNIEnv *env, unused jobject instance, jlong jcompositor, jint id, jint x, jint y) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);
	LOGV("JNI: touch down");

    b->post([b, id, x, y]() {
        b->touch_down(static_cast<uint32_t>(id), static_cast<uint32_t>(x), static_cast<uint32_t>(y));
    });
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchMotion)(unused JNIEnv *env, unused jobject instance, jlong jcompositor, jint id, jint x, jint y) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);
	LOGV("JNI: touch motion");

    b->post([b, id, x, y]() {
        b->touch_motion(static_cast<uint32_t>(id), static_cast<uint32_t>(x),
                             static_cast<uint32_t>(y));
    });
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchUp)(unused JNIEnv *env, unused jobject instance, jlong jcompositor, jint id) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);
	LOGV("JNI: touch up");

    b->post([b, id]() {
        b->touch_up(static_cast<uint32_t>(id));
    });
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchFrame)(unused JNIEnv *env, unused jobject instance, jlong jcompositor) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);
	LOGV("JNI: touch frame");

    b->post([b]() {
        b->touch_frame();
    });
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerMotion)(unused JNIEnv *env, unused jobject instance, jlong jcompositor, jint x, jint y) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);

    LOGV("JNI: pointer motion %dx%d", x, y);
    b->post([b, x, y](){
        b->pointer_motion(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
    });
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerScroll)(unused JNIEnv *env, unused jobject instance, jlong jcompositor, jint axis, jfloat value) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);

    LOGV("JNI: pointer scroll %d  %f", axis, value);
    b->post([b, axis, value]() {
        b->pointer_scroll(static_cast<uint32_t>(axis), value);
    });
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerButton)(unused JNIEnv *env, unused jobject instance, jlong jcompositor, jint button, jint type) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);

    LOGV("JNI: pointer button %d type %d", button, type);
    b->post([b, button, type]() {
        b->pointer_button(static_cast<uint32_t>(button), static_cast<uint32_t>(type));
    });
}

extern "C" void get_character_data(char** layout, int *shift, int *ec, char *ch);
extern "C" void android_keycode_get_eventcode(int kc, int *ec, int *shift);

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, keyboardKey)(JNIEnv *env, unused jobject instance,
                                           jlong jcompositor, jint type,
                                           jint key_code, jint jshift,
                                           jstring characters_) {
	if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);

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
        b->post([b]() {
            b->keyboard_key(42, WL_KEYBOARD_KEY_STATE_PRESSED); // Send KEY_LEFTSHIFT
        });

    // For some reason Android do not send ACTION_DOWN for non-English characters
    if (characters)
        b->post([b, event_code]() {
            b->keyboard_key(static_cast<uint32_t>(event_code), WL_KEYBOARD_KEY_STATE_PRESSED);
        });

    b->post([b, event_code, type]() {
        b->keyboard_key(static_cast<uint32_t>(event_code), static_cast<uint32_t>(type));
    });

    if (shift || jshift)
        b->post([b]() {
            b->keyboard_key(42, WL_KEYBOARD_KEY_STATE_RELEASED); // Send KEY_LEFTSHIFT
        });

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