# Copyright 2006 The Android Open Source Project

# XXX using libutils for simulator build only...
#
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := cpuburn-a53
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
ifdef TARGET_2ND_ARCH
LOCAL_SRC_FILES := 64bit/cpuburn-a53
else
LOCAL_SRC_FILES := 32bit/cpuburn-a53
endif
include $(BUILD_PREBUILT)
