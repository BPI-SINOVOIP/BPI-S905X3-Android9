LOCAL_PATH:= $(call my-dir)

# Build the Sample Embms Services
include $(CLEAR_VARS)

src_dirs := src

LOCAL_SRC_FILES := $(call all-java-files-under, $(src_dirs)) \
                   $(call all-Iaidl-files-under, aidl)

LOCAL_AIDL_INCLUDES := aidl/

LOCAL_PACKAGE_NAME := EmbmsMiddlewareCtsTestApp

LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := test_current
LOCAL_COMPATIBILITY_SUITE := cts vts
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_PROGUARD_ENABLED := disabled
include $(BUILD_CTS_PACKAGE)
