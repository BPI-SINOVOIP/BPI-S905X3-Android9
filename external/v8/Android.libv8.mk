##
LOCAL_PATH := $(call my-dir)
# libv8.a
# ===================================================
include $(CLEAR_VARS)

# Set up the target identity
LOCAL_MODULE := libv8
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_CFLAGS := -Wall -Werror

LOCAL_WHOLE_STATIC_LIBRARIES := libv8base libv8platform libv8sampler libv8src libv8gen v8peephole

LOCAL_EXPORT_C_INCLUDE_DIRS := \
	$(LOCAL_PATH)/include

LOCAL_MODULE_TARGET_ARCH_WARN := $(V8_SUPPORTED_ARCH)

include $(BUILD_STATIC_LIBRARY)


