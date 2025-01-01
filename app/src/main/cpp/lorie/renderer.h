#pragma once
#include <jni.h>
#include <android/hardware_buffer.h>
#include "lorie.h"

__unused int renderer_init(JNIEnv* env);
__unused void renderer_test_capabilities(int* legacy_drawing, uint8_t* flip);
__unused void renderer_set_buffer(JNIEnv* env, LorieBuffer* buffer);
__unused void renderer_set_window(JNIEnv* env, jobject surface);
__unused void renderer_set_shared_state(struct lorie_shared_server_state* state);
__unused int renderer_should_redraw(void);
__unused int renderer_redraw(void);
__unused void renderer_print_fps(float millis);
