LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    uevent.cpp

LOCAL_C_INCLUDES := \
    libnativehelper/include_jni

LOCAL_MODULE := libjniuevent

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libutils

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := bandwidth.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/etc/init/
else
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/init
endif

LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)


