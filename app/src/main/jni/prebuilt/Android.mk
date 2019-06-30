LOCAL_PATH := $(call my-dir)

# libs are prebuilt with termux
include $(CLEAR_VARS)
LOCAL_MODULE := android-support
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ffi
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := wayland-server
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)
