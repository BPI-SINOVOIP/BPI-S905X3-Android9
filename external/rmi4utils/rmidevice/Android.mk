LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := rmidevice
LOCAL_SRC_FILES := rmifunction.cpp rmidevice.cpp hiddevice.cpp
LOCAL_CFLAGS := \
    -Wall -Werror \
    -Wno-unused-parameter \

include $(BUILD_STATIC_LIBRARY)
