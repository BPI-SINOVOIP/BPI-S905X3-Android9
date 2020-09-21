LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := bt_vendor.conf
LOCAL_MODULE_CLASS := ETC

LOCAL_MODULE_TAGS := eng

LOCAL_SRC_FILES := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/etc/bluetooth
else
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/bluetooth
endif
include $(BUILD_PREBUILT)

