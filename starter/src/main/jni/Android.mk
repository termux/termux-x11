LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := x11-starter
LOCAL_SRC_FILES := starter.c exechelper.c
LOCAL_LDLIBS := -llog -ldl
include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/../../../../common/src/main/jni/Android.mk
