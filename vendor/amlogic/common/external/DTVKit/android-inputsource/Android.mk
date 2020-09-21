LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/companionlibrary/src/main/Android.mk \
    $(LOCAL_PATH)/logicdtvkit/src/main/Android.mk \
    $(LOCAL_PATH)/logicdtvkit/src/jni/Android.mk \
    $(LOCAL_PATH)/app/src/main/Android.mk