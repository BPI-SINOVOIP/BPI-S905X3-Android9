LOCAL_PATH := $(call my-dir)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
OUT_PATH := $(PRODUCT_OUT)/vendor
else
OUT_PATH := $(PRODUCT_OUT)/system
endif

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

ifeq ($(TARGET_ARCH), arm)
LOCAL_SRC_FILES := bin/tee_hdcp
else
LOCAL_SRC_FILES := bin64/tee_hdcp
endif

LOCAL_INIT_RC := tee_hdcp.rc
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE := tee_hdcp
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(OUT_PATH)/bin
include $(BUILD_PREBUILT)
