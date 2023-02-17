ROOT_PATH := $(call my-dir)
include $(ROOT_PATH)/lorie/Android.mk
include $(ROOT_PATH)/starter/Android.mk

LOCAL_PATH:= $(ROOT_PATH)/wayland/src

include $(CLEAR_VARS)
LOCAL_MODULE := wayland-server
LOCAL_SRC_FILES := \
	connection.c \
	event-loop.c \
	wayland-protocol.c \
	wayland-server.c \
	wayland-shm.c \
	wayland-os.c \
	wayland-util.c

#LOCAL_SRC_FILES += ../../lorie/utils/log.cpp
#LOCAL_CFLAGS := -finstrument-functions
#LOCAL_LDFLAGS := -llog

LOCAL_C_INCLUDES += $(LOCAL_PATH) $(LOCAL_PATH)/..
include $(BUILD_SHARED_LIBRARY)
