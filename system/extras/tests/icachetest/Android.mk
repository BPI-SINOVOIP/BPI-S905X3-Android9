# Copyright 2006 The Android Open Source Project
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= icache_main.cpp Profiler.cpp icache.S

LOCAL_SHARED_LIBRARIES := libc

LOCAL_MODULE:= icache

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_TARGET_ARCH := arm

LOCAL_CFLAGS += -Wall -Werror

include $(BUILD_EXECUTABLE)
