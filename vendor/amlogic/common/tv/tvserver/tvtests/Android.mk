# Copyright (c) 2014 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.
#
# Description: makefile

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LIB_TV_BINDER_PATH := $(LOCAL_PATH)/../../frameworks/libtvbinder

LOCAL_SRC_FILES:= \
    main.cpp

LOCAL_SHARED_LIBRARIES := \
    vendor.amlogic.hardware.tvserver@1.0 \
    libcutils \
    libutils \
    libbinder \
    libtvbinder \
    liblog

LOCAL_C_INCLUDES += \
    $(LIB_TV_BINDER_PATH)/include

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= tvtest
LOCAL_VENDOR_MODULE := true

include $(BUILD_EXECUTABLE)

