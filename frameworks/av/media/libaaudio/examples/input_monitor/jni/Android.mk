LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_C_INCLUDES := \
    $(call include-path-for, audio-utils) \
    frameworks/av/media/libaaudio/include \
    frameworks/av/media/libaaudio/src \
    frameworks/av/media/libaaudio/examples/utils

# NDK recommends using this kind of relative path instead of an absolute path.
LOCAL_SRC_FILES:= ../src/input_monitor.cpp
LOCAL_CFLAGS := -Wall -Werror
LOCAL_SHARED_LIBRARIES := libaaudio
LOCAL_MODULE := input_monitor
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_C_INCLUDES := \
    $(call include-path-for, audio-utils) \
    frameworks/av/media/libaaudio/include \
    frameworks/av/media/libaaudio/examples/utils

LOCAL_SRC_FILES:= ../src/input_monitor_callback.cpp
LOCAL_CFLAGS := -Wall -Werror
LOCAL_SHARED_LIBRARIES := libaaudio
LOCAL_MODULE := input_monitor_callback
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := libaaudio_prebuilt
LOCAL_SRC_FILES := libaaudio.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)
