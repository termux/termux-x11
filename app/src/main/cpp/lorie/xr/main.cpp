#include "base.h"
#include "input.h"
#include "math.h"
#include "renderer.h"

bool immersive = false;

Base* s_module_base = NULL;
Input* s_module_input = NULL;
Renderer* s_module_renderer = NULL;

#if defined(_DEBUG)
#include <GLES3/gl3.h>
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
		xrResultToString(s_module_base->GetInstance(), result, errorBuffer);
        ALOGE("OpenXR error on line %s:%d %s", file, line, errorBuffer);
	}
}
#endif

extern "C" {

void bindXrFramebuffer(void) {
    s_module_renderer->BindFramebuffer(s_module_base);
}

bool beginXrFrame(void) {
    if (s_module_renderer->InitFrame(s_module_base)) {

        // Set render canvas
        float distance = immersive ? 2.0f : 5.0f;
        s_module_renderer->SetConfigFloat(CONFIG_CANVAS_DISTANCE, distance);
        s_module_renderer->SetConfigInt(CONFIG_MODE, RENDER_MODE_MONO_SCREEN);
        s_module_renderer->SetConfigInt(CONFIG_PASSTHROUGH, !immersive);

        // Follow the view when immersive
        if (immersive) {
            s_module_renderer->Recenter(s_module_base);
        }

        // Update controllers state
        s_module_input->Update(s_module_base);

        // Lock framebuffer
        s_module_renderer->BeginFrame(0);

        return true;
    }
    return false;
}

void endXrFrame(void) {
    s_module_renderer->EndFrame();
    s_module_renderer->FinishFrame(s_module_base);
}

void enterXr(void) {
    s_module_base->EnterXR();
    s_module_input->Init(s_module_base);
    s_module_renderer->Init(s_module_base);
    ALOGV("EnterXr called");
}

void getXrResolution(int *width, int *height) {
    s_module_renderer->GetResolution(s_module_base, width, height);
}

bool isXrEnabled(void) {
    return s_module_base != NULL;
}

JNIEXPORT void JNICALL Java_com_termux_x11_XrActivity_init(JNIEnv *env, jobject obj) {

    // Do not allow second initialization
    if (s_module_base) {
        return;
    }

    // Allocate modules
    s_module_base = new Base();
    s_module_input = new Input();
    s_module_renderer = new Renderer();

    // Set platform flags
    s_module_base->SetPlatformFlag(PLATFORM_CONTROLLER_QUEST, true);
    s_module_base->SetPlatformFlag(PLATFORM_EXTENSION_PASSTHROUGH, true);
    s_module_base->SetPlatformFlag(PLATFORM_EXTENSION_PERFORMANCE, true);

    // Get Java VM
    JavaVM* vm;
    env->GetJavaVM(&vm);

    // Init XR
    xrJava java;
    java.vm = vm;
    java.activity = env->NewGlobalRef(obj);
    s_module_base->Init(&java, "termux-x11", 1);
    ALOGV("Init called");
}
}