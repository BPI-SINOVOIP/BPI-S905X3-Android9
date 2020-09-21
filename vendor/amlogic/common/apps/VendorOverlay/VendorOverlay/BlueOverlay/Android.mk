LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

CAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/overlay

#include files in res diretory
LOCAL_RESOURCE_DIR := \
    $(LOCAL_PATH)/res
LOCAL_SDK_VERSION := current
LOCAL_EXPORT_PACKAGE_RESOURCES := true

LOCAL_PACKAGE_NAME := BlueOverlay
LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)
