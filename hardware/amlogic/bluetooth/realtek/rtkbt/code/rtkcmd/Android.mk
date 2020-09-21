LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES += \
                    rtkcmd.c


LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := rtkcmd
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)

