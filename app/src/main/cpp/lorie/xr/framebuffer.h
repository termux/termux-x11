#pragma once

//TODO:remove once base.h is in C
#define XR_USE_PLATFORM_ANDROID 1
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/log.h>
#include <jni.h>
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, "OpenXR", __VA_ARGS__);
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "OpenXR", __VA_ARGS__);
#define GL(func) func;
#define OXR(func) func;

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <stdbool.h>

struct XrFramebuffer {
    int Width;
    int Height;
    bool Acquired;
    XrSwapchain Handle;

    uint32_t SwapchainIndex;
    uint32_t SwapchainLength;
    void* SwapchainImage;

    unsigned int* GLDepthBuffers;
    unsigned int* GLFrameBuffers;
};

bool XrFramebufferCreate(struct XrFramebuffer *framebuffer, XrSession session, int width, int height);
void XrFramebufferDestroy(struct XrFramebuffer *framebuffer);

void XrFramebufferAcquire(struct XrFramebuffer *framebuffer);
void XrFramebufferRelease(struct XrFramebuffer *framebuffer);
void XrFramebufferSetCurrent(struct XrFramebuffer *framebuffer);

#if XR_USE_GRAPHICS_API_OPENGL_ES
bool XrFramebufferCreateGL(struct XrFramebuffer *framebuffer, XrSession session, int width, int height);
#endif
