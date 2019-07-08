LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := jniLoader
LOCAL_SRC_FILES := dt_needed_main.c
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)
