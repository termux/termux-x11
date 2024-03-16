#include <string.h>
#include <GLES2/gl2.h>

#include "engine.h"
#include "input.h"
#include "math.h"
#include "renderer.h"

struct XrEngine xr_engine;
struct XrInput xr_input;
struct XrRenderer xr_renderer;
bool xr_immersive = false;
bool xr_initialized = false;

#if defined(_DEBUG)
#include <GLES2/gl2.h>
void GLCheckErrors(const char* file, int line) {
	for (int i = 0; i < 10; i++) {
		const GLenum error = glGetError();
		if (error == GL_NO_ERROR) {
			break;
		}
		ALOGE("OpenGL error on line %s:%d %d", file, line, error);
	}
}

void OXRCheckErrors(XrResult result, const char* file, int line) {
	if (XR_FAILED(result)) {
		char errorBuffer[XR_MAX_RESULT_STRING_SIZE];
		xrResultToString(xr_engine->Instance, result, errorBuffer);
        ALOGE("OpenXR error on line %s:%d %s", file, line, errorBuffer);
	}
}
#endif

bool isXrEnabled(void) {
    return xr_initialized;
}

JNIEXPORT void JNICALL Java_com_termux_x11_XrActivity_init(JNIEnv *env, jobject obj) {

    // Do not allow second initialization
    if (xr_initialized) {
        return;
    }

    // Set platform flags
    memset(&xr_engine, 0, sizeof(xr_engine));
    xr_engine.PlatformFlag[PLATFORM_CONTROLLER_QUEST] = true;
    xr_engine.PlatformFlag[PLATFORM_EXTENSION_PASSTHROUGH] = true;
    xr_engine.PlatformFlag[PLATFORM_EXTENSION_PERFORMANCE] = true;

    // Get Java VM
    JavaVM* vm;
    (*env)->GetJavaVM(env, &vm);

    // Init XR
    xrJava java;
    java.vm = vm;
    java.activity = (*env)->NewGlobalRef(env, obj);
    XrEngineInit(&xr_engine, &java, "termux-x11", 1);
    XrEngineEnter(&xr_engine);
    XrInputInit(&xr_engine, &xr_input);
    XrRendererInit(&xr_engine, &xr_renderer);
    xr_initialized = true;
    ALOGV("Init called");
}

JNIEXPORT void JNICALL
Java_com_termux_x11_XrActivity_render(JNIEnv *env, jobject obj) {
    if (XrRendererInitFrame(&xr_engine, &xr_renderer)) {

        // Set render canvas
        float distance = xr_immersive ? 2.0f : 5.0f;
        xr_renderer.ConfigFloat[CONFIG_CANVAS_DISTANCE] = distance;
        xr_renderer.ConfigInt[CONFIG_MODE] = RENDER_MODE_MONO_SCREEN;
        xr_renderer.ConfigInt[CONFIG_PASSTHROUGH] = !xr_immersive;

        // Follow the view when xr_immersive
        if (xr_immersive) {
            XrRendererRecenter(&xr_engine, &xr_renderer);
        }

        // Update controllers state
        XrInputUpdate(&xr_engine, &xr_input);

        // Lock framebuffer
        XrRendererBeginFrame(&xr_renderer, 0);

        glClearColor(0, 0, 1, 0.25f);
        glClear(GL_COLOR_BUFFER_BIT);
        //TODO:render frame

        // Unlock framebuffer
        XrRendererEndFrame(&xr_renderer);
        XrRendererFinishFrame(&xr_engine, &xr_renderer);
    }
}