### GENERATED, do not edit
### for changes, see genmakefiles.py
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/Android.v8common.mk
LOCAL_MODULE := libv8sampler
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_SRC_FILES := \
	src/libsampler/sampler.cc
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/include
include $(BUILD_STATIC_LIBRARY)
