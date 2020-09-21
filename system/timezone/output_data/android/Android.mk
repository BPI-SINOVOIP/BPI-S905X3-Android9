LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := tzlookup.xml
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/share/zoneinfo
include $(BUILD_PREBUILT)

ifeq ($(WITH_HOST_DALVIK),true)

# A host version of the tzlookup.xml module for use by
# hostdex rules.

include $(CLEAR_VARS)
LOCAL_MODULE := tzlookup.xml
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_IS_HOST_MODULE := true
LOCAL_SRC_FILES := tzlookup.xml
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_STEM := $(LOCAL_SRC_FILES)
LOCAL_MODULE_PATH := $(HOST_OUT)/usr/share/zoneinfo
include $(BUILD_PREBUILT)

endif
