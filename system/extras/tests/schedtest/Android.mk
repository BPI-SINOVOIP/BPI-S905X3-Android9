LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	schedtest.c

LOCAL_MODULE := schedtest
LOCAL_CFLAGS := -Wno-unused-parameter -Wall -Werror

include $(BUILD_EXECUTABLE)
