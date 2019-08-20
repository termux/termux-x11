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
#include <xkbcommon/xkbcommon.h>

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
	void layout_change_callback(char *layout);

	void on_egl_init();
	void on_egl_uninit();
	
	LorieEGLHelper helper;

	struct xkb_context *xkb_context = nullptr;
	struct xkb_rule_names xkb_names = {0};
	struct xkb_keymap *xkb_keymap = nullptr;

    std::thread self;
};

LorieBackendAndroid::LorieBackendAndroid()
    : self(&LorieCompositor::start, this) {}



void LorieBackendAndroid::on_egl_init() {
	renderer.init();
}

void LorieBackendAndroid::on_egl_uninit() {
	renderer.uninit();
}

void LorieBackendAndroid::backend_init() {
	if (!helper.init(EGL_DEFAULT_DISPLAY)) {
	    LOGE("Failed to initialize EGL context");
	}

	helper.onInit = std::bind(std::mem_fn(&LorieBackendAndroid::on_egl_init), this);
	//helper.onUninit = std::bind(std::mem_fn(&LorieBackendAndroid::on_egl_uninit), this);

	if (xkb_context == nullptr) {
		xkb_context = xkb_context_new((enum xkb_context_flags) 0);
		if (xkb_context == nullptr) {
			LOGE("failed to create XKB context\n");
			return;
		}
	}
	
	xkb_names.rules = strdup("evdev");
	xkb_names.model = strdup("pc105");
	xkb_names.layout = strdup("us");
	
	xkb_keymap = xkb_keymap_new_from_names(xkb_context, &xkb_names, (enum xkb_keymap_compile_flags) 0);
	if (xkb_keymap == nullptr) {
		LOGE("failed to compile global XKB keymap\n");
		LOGE("  tried rules %s, model %s, layout %s, variant %s, "
			"options %s\n",
			xkb_names.rules, xkb_names.model,
			xkb_names.layout, xkb_names.variant,
			xkb_names.options);
		return;
	}
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
	LOGI("Locale: %s", xkb_names.layout);
	char *string = xkb_keymap_get_as_string (xkb_keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
	*size = static_cast<int>(strlen (string) + 1);
	*fd = os_create_anonymous_file(static_cast<size_t>(*size));
	char *map = (char*) mmap (nullptr, static_cast<size_t>(*size), PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
	strcpy (map, string);
	munmap (map, static_cast<size_t>(*size));
	free (string);
}

void LorieBackendAndroid::window_change_callback(EGLNativeWindowType win, uint32_t width, uint32_t height, uint32_t physical_width, uint32_t physical_height) {
	LOGE("WindowChangeCallback");
	helper.setWindow(win);
	output_resize(width, height, physical_width, physical_height);
}

void LorieBackendAndroid::layout_change_callback(char *layout) {
	xkb_keymap_unref(xkb_keymap);
	xkb_names.layout = layout;

	xkb_keymap = xkb_keymap_new_from_names(xkb_context, &xkb_names, (xkb_keymap_compile_flags)0);
	if (xkb_keymap == NULL) {
		LOGE("failed to compile global XKB keymap\n");
		LOGE("  tried rules %s, model %s, layout %s, variant %s, "
				"options %s\n",
				xkb_names.rules, xkb_names.model,
				xkb_names.layout, xkb_names.variant,
				xkb_names.options);
		return;
	}

	keyboard_keymap_changed();
}

///////////////////////////////////////////////////////////

#define JNI_DECLARE_INNER(package, classname, methodname ) \
     Java_ ## package ## _ ## classname  ## _ ## methodname
#define JNI_DECLARE(classname, methodname) \
     JNI_DECLARE_INNER(com_termux_x11, classname, methodname)

#define WL_POINTER_MOTION 2

static LorieBackendAndroid* fromLong(jlong v) {
    union {
        jlong l;
        LorieBackendAndroid* b;
    } u = {0};
    u.l = v;
    return u.b;
}

extern "C" JNIEXPORT jlong JNICALL
JNI_DECLARE(LorieService, createLorieThread)(JNIEnv __unused *env, jobject __unused instance) {
	setenv("XDG_RUNTIME_DIR", "/data/data/com.termux/files/usr/tmp", 1);
	return (jlong) new LorieBackendAndroid;
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, terminate)(JNIEnv __unused *env, jobject __unused instance,  jlong jcompositor) {
	if (jcompositor == 0) return;
    	LorieBackendAndroid *b = fromLong(jcompositor);
	LOGI("JNI: requested termination");
    b->terminate();
    b->self.join();
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, windowChanged)(JNIEnv *env, jobject __unused instance, jlong jcompositor, jobject jsurface, jint width, jint height, jint mmWidth, jint mmHeight) {
	if (jcompositor == 0) return;
	LorieBackendAndroid *b = fromLong(jcompositor);

	EGLNativeWindowType win = ANativeWindow_fromSurface(env, jsurface);
	b->queue.call(&LorieBackendAndroid::window_change_callback, b, win, width, height, mmWidth, mmHeight);

    LOGV("JNI: window is changed: %p(%p) %dx%d (%dmm x %dmm)", win, jsurface, width, height, mmWidth, mmHeight);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchDown)(JNIEnv __unused *env, jobject __unused instance, jlong jcompositor, jint id, jint x, jint y) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);
	LOGV("JNI: touch down");

    b->touch_down(static_cast<uint32_t>(id), static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchMotion)(JNIEnv __unused *env, jobject __unused instance, jlong jcompositor, jint id, jint x, jint y) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);
	LOGV("JNI: touch motion");

    b->touch_motion(static_cast<uint32_t>(id), static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchUp)(JNIEnv __unused *env, jobject __unused instance, jlong jcompositor, jint id) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);
	LOGV("JNI: touch up");

    b->touch_up(static_cast<uint32_t>(id));
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, touchFrame)(JNIEnv __unused *env, jobject __unused instance, jlong jcompositor) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);
	LOGV("JNI: touch frame");

    b->touch_frame();
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerMotion)(JNIEnv __unused *env, jobject __unused instance, jlong jcompositor, jint x, jint y) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);

    LOGV("JNI: pointer motion %dx%d", x, y);
    b->pointer_motion(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerScroll)(JNIEnv __unused *env, jobject __unused instance, jlong jcompositor, jint axis, jfloat value) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);

    LOGV("JNI: pointer scroll %d  %f", axis, value);
    b->pointer_scroll(static_cast<uint32_t>(axis), value);
}

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, pointerButton)(JNIEnv __unused *env, jobject __unused instance, jlong jcompositor, jint button, jint type) {
    if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);

    LOGV("JNI: pointer button %d type %d", button, type);
    b->pointer_button(static_cast<uint32_t>(button), static_cast<uint32_t>(type));
}

extern "C" void get_character_data(char** layout, int *shift, int *ec, char *ch);
extern "C" void android_keycode_get_eventcode(int kc, int *ec, int *shift);

extern "C" JNIEXPORT void JNICALL
JNI_DECLARE(LorieService, keyboardKey)(JNIEnv *env, jobject __unused instance,
                                           jlong jcompositor, jint type,
                                           jint key_code, jint jshift,
                                           jstring characters_) {
	if (jcompositor == 0) return;
    LorieBackendAndroid *b = fromLong(jcompositor);

    char *characters = NULL;

	int event_code = 0;
    int shift = jshift;
	if (characters_ != NULL) characters = (char*) env->GetStringUTFChars(characters_, 0);
    if (key_code && !characters) {
		android_keycode_get_eventcode(key_code, &event_code, &shift);
		LOGE("kc: %d ec: %d", key_code, event_code);
		if (strcmp(b->xkb_names.layout, "us") && event_code != 0) {
			b->queue.call(&LorieBackendAndroid::layout_change_callback, b, (char*)"us");
		}
    }
    if (!key_code && characters) {
        char *layout = NULL;
        get_character_data(&layout, &shift, &event_code, characters);
        if (layout && b->xkb_names.layout != layout) {
			b->queue.call(&LorieBackendAndroid::layout_change_callback, b, layout);
        }
    }
	LOGE("Keyboard input: keyCode: %d; eventCode: %d; characters: %s; shift: %d, type: %d", key_code, event_code, characters, shift, type);
	//if (!event_code) {
	    //lorie_clipboard_set_event(backend, characters);
        //lorie_key_event(backend, (uint8_t) WL_KEYBOARD_KEY_STATE_PRESSED, (uint16_t) 42); // Send KEY_LEFTSHIFT
        //lorie_key_event(backend, (uint8_t) WL_KEYBOARD_KEY_STATE_PRESSED, (uint16_t) 110); // Send KEY_INSERT
        //lorie_key_event(backend, (uint8_t) WL_KEYBOARD_KEY_STATE_RELEASED, (uint16_t) 110); // Send KEY_INSERT
        //lorie_key_event(backend, (uint8_t) WL_KEYBOARD_KEY_STATE_RELEASED, (uint16_t) 42); // Send KEY_LEFTSHIFT
        //lorie_clipboard_set_event(backend, NULL);
	//}

    if (shift || jshift)
        b->keyboard_key(42, WL_KEYBOARD_KEY_STATE_PRESSED); // Send KEY_LEFTSHIFT

    // For some reason Android do not send ACTION_DOWN for non-English characters
    if (characters)
        b->keyboard_key(static_cast<uint32_t>(event_code), WL_KEYBOARD_KEY_STATE_PRESSED);

	b->keyboard_key(static_cast<uint32_t>(event_code), static_cast<uint32_t>(type));

    if (shift || jshift)
        b->keyboard_key(42, WL_KEYBOARD_KEY_STATE_RELEASED); // Send KEY_LEFTSHIFT

    if (characters_ != NULL) env->ReleaseStringUTFChars(characters_, characters);
}
