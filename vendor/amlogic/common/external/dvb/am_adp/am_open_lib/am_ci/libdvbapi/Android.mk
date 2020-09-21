LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libdvbapi
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := dvbapi.c

LOCAL_CFLAGS+= -DLOG_LEVEL=1 # FIXME
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/.. $(LOCAL_PATH)/../../include

LOCAL_SHARED_LIBRARIES += liblog libc

LOCAL_PRELINK_MODULE := false

include $(BUILD_STATIC_LIBRARY)

