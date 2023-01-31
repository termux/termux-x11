LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := lorie
LOCAL_SRC_FILES := \
	compositor.cpp \
	egl-helper.cpp \
	message-queue.cpp \
	renderer.cpp \
	utils/log.cpp \
	scanner/wayland.cpp \
	\
	backend/android/android-app.cpp \
	backend/android/utils.c \
	lorie_wayland_server.cpp \
	$(wildcard $(WAYLAND_GENERATED)/*.cpp)

LOCAL_CFLAGS := -finstrument-functions
LOCAL_CXXFLAGS := -std=c++20
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/scanner $(LOCAL_PATH)/../prebuilt/include $(WAYLAND_GENERATED)
LOCAL_LDLIBS := -lEGL -lGLESv2 -llog -landroid
LOCAL_LDFLAGS := -L$(LOCAL_PATH)/../prebuilt/$(TARGET_ARCH_ABI) -lwayland-server
include $(BUILD_SHARED_LIBRARY)
