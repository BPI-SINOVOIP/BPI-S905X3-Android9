# Copyright (C) 2014 The Android Open Source Project
# Copyright (C) 2011 Amlogic
#
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
DVB_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)/dvb)
LIB_TV_BINDER_PATH := $(BOARD_AML_VENDOR_PATH)/tv/frameworks/libtvbinder
LIB_SQLITE_PATH := $(wildcard external/sqlite)


ifneq (,$(wildcard hardware/amlogic/gralloc))
	GRALLOC_DIR := hardware/amlogic/gralloc
else
	GRALLOC_DIR := hardware/libhardware/modules/gralloc
endif

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := \
    vendor.amlogic.hardware.tvserver@1.0 \
    libcutils \
    libutils \
    libtvbinder \
    libbinder \
    libui \
    liblog \
    libhardware

LOCAL_REQUIRED_MODULES := libtvbinder

LOCAL_CFLAGS += -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_SRC_FILES := \
    tv_input.cpp \
    TvInputIntf.cpp

LOCAL_MODULE := tv_input.amlogic
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += \
    $(LIB_TV_BINDER_PATH)/include	\
    $(LIB_SQLITE_PATH)/dist \
    system/media/audio_effects/include \
    system/core/libion/include \
    system/core/libion/kernel-headers \
    hardware/amlogic/gralloc \
    hardware/amlogic/screen_source \
    hardware/amlogic/audio/libTVaudio \
    frameworks/native/libs/nativewindow/include \
    $(GRALLOC_DIR)

LOCAL_C_INCLUDES += \
   external/libcxx/include

LOCAL_CPPFLAGS += -std=c++14

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
