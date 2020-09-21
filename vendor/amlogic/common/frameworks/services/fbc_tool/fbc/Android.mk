# Copyright (c) 2014 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.
#
# Description: makefile

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
  FBCMain.cpp

LOCAL_C_INCLUDES += \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/fbc_tool/libfbc/include

LOCAL_SHARED_LIBRARIES := \
  libcutils \
  liblog \
  libc \
  libfbc \
  libutils

LOCAL_MODULE := fbc

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_EXECUTABLE)
