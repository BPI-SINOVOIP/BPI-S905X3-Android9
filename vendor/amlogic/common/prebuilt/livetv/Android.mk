ifneq ($(wildcard packages/apps/TV/Android.mk),packages/apps/TV/Android.mk)
LOCAL_PATH := $(call my-dir)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
OUT_PATH := $(TARGET_OUT_VENDOR)
else
OUT_PATH := $(TARGET_OUT)/
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libtunertvinput_jni
LOCAL_MULTILIB := 32
LOCAL_MODULE_SUFFIX :=.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(OUT_PATH)/lib
LOCAL_SRC_FILES := lib/libtunertvinput_jni.so
include $(BUILD_PREBUILT)
endif