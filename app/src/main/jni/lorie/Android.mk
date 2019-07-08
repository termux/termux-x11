LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := lorie
LOCAL_SRC_FILES := \
	main.c \
	locale/android-utils.c \
	backend-android.c \
	renderer.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../prebuilt/include
LOCAL_LDLIBS := -lEGL -lGLESv2 -llog -landroid
LOCAL_LDFLAGS := -L$(LOCAL_PATH)/../prebuilt/$(TARGET_ARCH_ABI) -lwayland-server
LOCAL_SHARED_LIBRARIES := xkbcommon
include $(BUILD_SHARED_LIBRARY)
