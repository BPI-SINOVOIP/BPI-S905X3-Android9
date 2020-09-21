LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := PartnerSupportSampleCustomization
LOCAL_MODULE_TAGS := optional

LOCAL_PROGUARD_ENABLED := disabled
# Overlay view related functionality requires system APIs.
LOCAL_SDK_VERSION := system_current
LOCAL_MIN_SDK_VERSION := 23  # M

# Required for customization
LOCAL_PRIVILEGED_MODULE := true

LOCAL_RESOURCE_DIR := \
    $(LOCAL_PATH)/res

include $(BUILD_PACKAGE)
