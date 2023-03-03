LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := lorie
LOCAL_SRC_FILES := \
	compositor.cpp \
	lorie_wayland_server.cpp \
	$(wildcard $(WAYLAND_GENERATED)/*.cpp)

LOCAL_CXXFLAGS := -std=c++20
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/../wayland/src $(LOCAL_PATH)/../wayland $(WAYLAND_GENERATED)
LOCAL_LDLIBS := -lEGL -lGLESv2 -llog -landroid
LOCAL_SHARED_LIBRARIES := wayland-server
include $(BUILD_SHARED_LIBRARY)
