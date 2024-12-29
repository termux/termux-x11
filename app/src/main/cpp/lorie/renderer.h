#pragma once
#include <jni.h>
#include <android/hardware_buffer.h>
#include "lorie.h"

__unused int renderer_init(JNIEnv* env, int* legacy_drawing, uint8_t* flip);
__unused void renderer_set_buffer(JNIEnv* env, AHardwareBuffer* buffer);
__unused void renderer_set_window(JNIEnv* env, jobject surface);
__unused void renderer_set_shared_state(struct lorie_shared_server_state* state);
__unused int renderer_should_redraw(void);
__unused int renderer_redraw(JNIEnv* env, uint8_t flip);
__unused void renderer_print_fps(float millis);

__unused void renderer_update_root(int w, int h, void* data, uint8_t flip);

#define AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM 5 // Stands to HAL_PIXEL_FORMAT_BGRA_8888