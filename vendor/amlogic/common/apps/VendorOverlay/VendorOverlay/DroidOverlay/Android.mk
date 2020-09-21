LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/overlay
#include files in res diretory
LOCAL_RESOURCE_DIR := \
    $(LOCAL_PATH)/res
LOCAL_SDK_VERSION := current
LOCAL_EXPORT_PACKAGE_RESOURCES := true

LOCAL_PACKAGE_NAME := DroidOverlay
LOCAL_CERTIFICATE := platform
LOCAL_POST_INSTALL_CMD := \
  cp $(TARGET_OUT_VENDOR)/overlay/DroidOverlay/DroidOverlay.apk $(TARGET_OUT_VENDOR)/overlay/DroidOverlay.apk; \
  rm -rf $(TARGET_OUT_VENDOR)/overlay/DroidOverlay
include $(BUILD_PACKAGE)
