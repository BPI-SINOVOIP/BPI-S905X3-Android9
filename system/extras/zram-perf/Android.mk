LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := zram-perf
LOCAL_CFLAGS += -g -Wall -Werror -Wno-missing-field-initializers -Wno-sign-compare -Wno-unused-parameter
LOCAL_SRC_FILES := \
    zram-perf.cpp
include $(BUILD_EXECUTABLE)
