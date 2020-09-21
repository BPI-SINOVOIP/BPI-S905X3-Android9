# Copyright (c) 2014 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.
#
# Description: makefile

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LIB_VENDOR := $(wildcard $(BOARD_AML_VENDOR_PATH))
LIB_SQLITE_PATH := $(wildcard external/sqlite)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  CFile.cpp \
  CTvLog.cpp \
  CMsgQueue.cpp \
  CSqlite.cpp \
  serial_base.cpp \
  serial_operate.cpp \
  tvutils.cpp \
  zepoll.cpp \
  tvconfig/CIniFile.cpp \
  tvconfig/tvconfig.cpp \
  tvconfig/tvscanconfig.cpp

LOCAL_C_INCLUDES += \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol/PQ/include \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services \
  $(LIB_SQLITE_PATH)/dist \
  $(BOARD_AML_VENDOR_PATH)/tv/tvserver/libtv/include \
  frameworks/native/libs/binder/include \
  external/jsoncpp/include

LOCAL_SHARED_LIBRARIES += \
  vendor.amlogic.hardware.systemcontrol@1.0 \
  vendor.amlogic.hardware.systemcontrol@1.1 \
  libsystemcontrolservice \
  liblog

LOCAL_STATIC_LIBRARIES += \
  libjsoncpp

LOCAL_MODULE:= libtv_utils

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

#DVB define
ifeq ($(BOARD_HAS_ADTV),true)
LOCAL_CFLAGS += -DSUPPORT_ADTV
endif

include $(BUILD_STATIC_LIBRARY)
