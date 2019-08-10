LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := lorie
LOCAL_SRC_FILES := \
	client.cpp \
	clipboard.cpp \
	compositor.cpp \
	egl-helper.cpp \
	message-queue.cpp \
	renderer.cpp \
	seat.cpp \
	surface.cpp \
	output.cpp \
	utils/log.cpp \
	utils/utils.cpp \
	scanner/wayland.cpp \
	\
	backend/android/android-app.cpp \
	backend/android/utils.c

LOCAL_CFLAGS := -finstrument-functions
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/scanner $(LOCAL_PATH)/../prebuilt/include
LOCAL_LDLIBS := -lEGL -lGLESv2 -llog -landroid
LOCAL_LDFLAGS := -L$(LOCAL_PATH)/../prebuilt/$(TARGET_ARCH_ABI) -lwayland-server
LOCAL_SHARED_LIBRARIES := xkbcommon
include $(BUILD_SHARED_LIBRARY)
