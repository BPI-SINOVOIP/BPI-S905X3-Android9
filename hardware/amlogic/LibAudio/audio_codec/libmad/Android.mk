LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES += libutils libz libbinder libdl libcutils libc liblog

LOCAL_MODULE    := libmad
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS  += -Werror -Wmissing-field-initializers -Wunused-parameter -Wunused-variable -Wsign-compare

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES += libutils libz libbinder libdl libcutils libc liblog

LOCAL_MODULE    := libmad_sys
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS  += -Werror -Wmissing-field-initializers -Wunused-parameter -Wunused-variable -Wsign-compare

include $(BUILD_SHARED_LIBRARY)