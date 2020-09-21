# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := timetest.c
LOCAL_MODULE := timetest
LOCAL_CFLAGS := -Wall -Werror
LOCAL_MODULE_TAGS := tests
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := libc
include $(BUILD_NATIVE_TEST)
