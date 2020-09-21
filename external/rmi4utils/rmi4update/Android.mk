LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := rmi4update
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../rmidevice
LOCAL_SRC_FILES := main.cpp rmi4update.cpp updateutil.cpp firmware_image.cpp
LOCAL_CFLAGS := \
    -Wall -Werror \
    -Wno-unused-parameter \

LOCAL_STATIC_LIBRARIES := rmidevice

include $(BUILD_EXECUTABLE)
