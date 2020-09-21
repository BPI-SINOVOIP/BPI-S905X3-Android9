### GENERATED, do not edit
### for changes, see genmakefiles.py
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/Android.v8common.mk
LOCAL_MODULE := libv8base
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_SRC_FILES := \
	src/base/bits.cc \
	src/base/cpu.cc \
	src/base/debug/stack_trace.cc \
	src/base/division-by-constant.cc \
	src/base/file-utils.cc \
	src/base/functional.cc \
	src/base/ieee754.cc \
	src/base/logging.cc \
	src/base/once.cc \
	src/base/platform/condition-variable.cc \
	src/base/platform/mutex.cc \
	src/base/platform/platform-posix.cc \
	src/base/platform/semaphore.cc \
	src/base/platform/time.cc \
	src/base/sys-info.cc \
	src/base/utils/random-number-generator.cc
LOCAL_SRC_FILES += \
	src/base/debug/stack_trace_android.cc \
	src/base/platform/platform-linux.cc
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/Android.v8common.mk
LOCAL_MODULE := libv8base
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_SRC_FILES := \
	src/base/bits.cc \
	src/base/cpu.cc \
	src/base/debug/stack_trace.cc \
	src/base/division-by-constant.cc \
	src/base/file-utils.cc \
	src/base/functional.cc \
	src/base/ieee754.cc \
	src/base/logging.cc \
	src/base/once.cc \
	src/base/platform/condition-variable.cc \
	src/base/platform/mutex.cc \
	src/base/platform/platform-posix.cc \
	src/base/platform/semaphore.cc \
	src/base/platform/time.cc \
	src/base/sys-info.cc \
	src/base/utils/random-number-generator.cc
ifeq ($(HOST_OS),linux)
LOCAL_SRC_FILES += \
	src/base/debug/stack_trace_posix.cc \
	src/base/platform/platform-linux.cc
endif
ifeq ($(HOST_OS),darwin)
LOCAL_SRC_FILES += \
	src/base/debug/stack_trace_posix.cc \
	src/base/platform/platform-macos.cc
endif
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src
include $(BUILD_HOST_STATIC_LIBRARY)
