LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := sane_schedstat.c

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := sane_schedstat
LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter

include $(BUILD_EXECUTABLE)
