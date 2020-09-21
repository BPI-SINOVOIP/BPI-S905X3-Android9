# Copyright (C) 2013 Amlogic
#
#

LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# /system/lib/hw/screen_source.amlogic.so
include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := aml_screen.cpp v4l2_vdin.cpp

MESON_GRALLOC_DIR ?= hardware/amlogic/gralloc

LOCAL_C_INCLUDES += \
    frameworks/native/include/utils \
    frameworks/native/include \
    frameworks/native/include/android \
    frameworks/native/libs/nativewindow/include \
    frameworks/av/include/media \
    system/core/include/utils \
    system/core/libion/include \
    system/core/libion/kernel-headers \
    $(MESON_GRALLOC_DIR)

LOCAL_SHARED_LIBRARIES:= libutils liblog libui libcutils

LOCAL_MODULE := screen_source.amlogic
LOCAL_CFLAGS:= -DLOG_TAG=\"screen_source\"

LOCAL_KK=0
ifeq ($(GPU_TYPE),t83x)
LOCAL_KK:=1
endif
ifeq ($(GPU_ARCH),midgard)
LOCAL_KK:=1
endif
ifeq ($(LOCAL_KK),1)
	LOCAL_CFLAGS += -DMALI_AFBC_GRALLOC=1
else
	LOCAL_CFLAGS += -DMALI_AFBC_GRALLOC=0
endif

LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
