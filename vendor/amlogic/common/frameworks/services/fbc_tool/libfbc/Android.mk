# Copyright (c) 2014 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.
#
# Description: makefile

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  CFbcCommunication.cpp \
  CFbcLinker.cpp \
  CFbcProtocol.cpp \
  CFbcUpgrade.cpp \
  CSerialPort.cpp \
  CVirtualInput.cpp \
  CFbcLog.cpp \
  CFile.cpp \
  CFbcEpoll.cpp

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/include \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol/PQ/include

LOCAL_SHARED_LIBRARIES := \
  libcutils \
  liblog \
  libutils \
  libsystemcontrolservice \
  vendor.amlogic.hardware.systemcontrol@1.1 \
  libbinder

LOCAL_MODULE:= libfbc

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
