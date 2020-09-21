### GENERATED, do not edit
### for changes, see genmakefiles.py
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/Android.v8common.mk
LOCAL_MODULE := libv8platform
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_SRC_FILES := \
	src/libplatform/default-platform.cc \
	src/libplatform/task-queue.cc \
	src/libplatform/tracing/trace-buffer.cc \
	src/libplatform/tracing/trace-config.cc \
	src/libplatform/tracing/trace-object.cc \
	src/libplatform/tracing/trace-writer.cc \
	src/libplatform/tracing/tracing-controller.cc \
	src/libplatform/worker-thread.cc
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/include
include $(BUILD_STATIC_LIBRARY)
