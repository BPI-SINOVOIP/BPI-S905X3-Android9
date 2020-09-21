LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE := libkeystonecorrection
#LOCAL_MULTILIB := both
LOCAL_32_BIT_ONLY := true
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_arm := lib/libkeystonecorrection.so
#LOCAL_SRC_FILES_arm64 := lib64/libkeystonecorrection.so

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib/
#LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64/
else
LOCAL_MODULE_PATH_32 := $(TARGET_OUT)/lib/
#LOCAL_MODULE_PATH_64 := $(TARGET_OUT)/lib64/
endif

include $(BUILD_PREBUILT)

