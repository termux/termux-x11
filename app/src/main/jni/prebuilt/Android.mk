LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 
LOCAL_MODULE := android-support
LOCAL_SRC_FILES:= $(TARGET_ARCH_ABI)/libandroid-support.so 
include $(PREBUILT_SHARED_LIBRARY) 

include $(CLEAR_VARS) 
LOCAL_MODULE := ffi
LOCAL_SRC_FILES:= $(TARGET_ARCH_ABI)/libffi.so 
include $(PREBUILT_SHARED_LIBRARY) 

include $(CLEAR_VARS) 
LOCAL_MODULE := wayland-server
LOCAL_SRC_FILES:= $(TARGET_ARCH_ABI)/libwayland-server.so 
include $(PREBUILT_SHARED_LIBRARY) 
