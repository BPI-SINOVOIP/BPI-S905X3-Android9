# Copyright 2015 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := mksquashfsimage.sh
LOCAL_SRC_FILES := mksquashfsimage.sh
LOCAL_MODULE_CLASS := EXECUTABLES
# We don't need any additional suffix.
LOCAL_MODULE_SUFFIX :=
LOCAL_BUILT_MODULE_STEM := $(notdir $(LOCAL_SRC_FILES))
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_HOST_OS := linux darwin
include $(BUILD_PREBUILT)
