LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := lorie
LOCAL_SRC_FILES := \
	main.c \
	backend-android.c \
	renderer.c
LOCAL_SHARED_LIBRARIES := wayland-server
LOCAL_LDLIBS := -lEGL -lGLESv2 -llog -landroid
include $(BUILD_SHARED_LIBRARY)
