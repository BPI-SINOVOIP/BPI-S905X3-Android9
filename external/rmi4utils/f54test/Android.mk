LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := f54test
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../rmidevice
LOCAL_SRC_FILES := main.cpp f54test.cpp testutil.cpp display.cpp
LOCAL_CFLAGS := \
    -Wall -Werror \
    -Wno-sometimes-uninitialized \
    -Wno-unused-parameter \

LOCAL_STATIC_LIBRARIES := rmidevice

include $(BUILD_EXECUTABLE)
