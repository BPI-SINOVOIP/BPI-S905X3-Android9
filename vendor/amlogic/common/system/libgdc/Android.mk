LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := gdc.c IONmem.c
LOCAL_MODULE := libgdc
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog libion
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
	system/core/libion/include/ \
	system/core/libion/kernel-headers \
	system/core/liblog/include \
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_CFLAGS := -Werror

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE:= false

LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= gdc_test.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include \
	system/core/libion/include/ \
	system/core/libion/kernel-headers \
	system/core/liblog/include
LOCAL_SHARED_LIBRARIES := liblog libion libgdc

LOCAL_CFLAGS += -g
LOCAL_CPPFLAGS := -g

LOCAL_MODULE := gdc_test

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_EXECUTABLE)
