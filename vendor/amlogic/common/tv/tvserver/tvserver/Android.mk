# Copyright (c) 2014 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.
#
# Description: makefile

LOCAL_PATH:= $(call my-dir)


DVB_PATH := $(wildcard external/dvb)
ifeq ($(DVB_PATH), )
  DVB_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)/external/dvb)
endif
ifeq ($(DVB_PATH), )
  DVB_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)/dvb)
endif

LIB_ZVBI_PATH := $(wildcard external/libzvbi)
ifeq ($(LIB_ZVBI_PATH), )
  LIB_ZVBI_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)/external/libzvbi)
endif

LIB_TV_UTILS := $(LOCAL_PATH)/../tvutils
LIB_TV_BINDER := $(LOCAL_PATH)/../../frameworks/libtvbinder

AM_LIBPLAYER_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)/frameworks/av/LibPlayer)
LIB_SQLITE_PATH := $(wildcard external/sqlite)


#tvserver
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
  main_tvserver.cpp \
  DroidTvServer.cpp \
  DroidTvServiceIntf.cpp \
  MemoryLeakTrackUtil.cpp

LOCAL_SHARED_LIBRARIES += \
  vendor.amlogic.hardware.tvserver@1.0 \
  vendor.amlogic.hardware.systemcontrol@1.0 \
  vendor.amlogic.hardware.systemcontrol@1.1 \
  libbase \
  libhidlbase \
  libhidltransport \
  libutils \
  libbinder \
  libcutils \
  liblog \
  libtvbinder \
  libtv

LOCAL_C_INCLUDES := \
  system/libhidl/transport/include/hidl \
  system/libhidl/libhidlmemory/include \
  hardware/libhardware/include \
  hardware/libhardware_legacy/include \
  $(LOCAL_PATH)/../libtv \
  $(LOCAL_PATH)/../libtv/tvdb \
  $(LOCAL_PATH)/../libtv/tv \
  $(LOCAL_PATH)/../libtv/include \
  $(LOCAL_PATH)/../libtv/gpio \
  $(LOCAL_PATH)/../tvfbclinker/include \
  $(LIB_TV_UTILS)/include \
  $(LIB_SQLITE_PATH)/dist

LOCAL_C_INCLUDES += \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol/PQ/include \
  system/extras/ext4_utils \
  $(LIB_TV_BINDER)/include \
  system/media/audio_effects/include \
  hardware/amlogic/audio/libTVaudio

ifeq ($(wildcard hardware/amlogic/media),hardware/amlogic/media)
$(info "have hardware/amlogic/media")
AML_DEC_PATH := $(wildcard hardware/amlogic/media)
LOCAL_C_INCLUDES += \
  $(AML_DEC_PATH)/amadec/include \
  $(AML_DEC_PATH)/amcodec/include
else
AML_DEC_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)/frameworks/av/LibPlayer)
LOCAL_C_INCLUDES += \
  $(AML_DEC_PATH)/amadec/include \
  $(AML_DEC_PATH)/amcodec/include \
  $(AML_DEC_PATH)/amffmpeg \
  $(AML_DEC_PATH)/amplayer
endif

LOCAL_CFLAGS += -DTARGET_BOARD_$(strip $(TVAPI_TARGET_BOARD_VERSION))

#DVB define
ifeq ($(BOARD_HAS_ADTV),true)
LOCAL_CFLAGS += -DSUPPORT_ADTV

LOCAL_SHARED_LIBRARIES += \
  libam_mw \
  libam_adp \
  libam_ver

LOCAL_C_INCLUDES += \
  $(DVB_PATH)/include/am_adp \
  $(DVB_PATH)/include/am_mw \
  $(DVB_PATH)/include/am_ver \
  $(DVB_PATH)/android/ndk/include \
  $(LIB_ZVBI_PATH)/ntsc_decode/include \
  $(LIB_ZVBI_PATH)/ntsc_decode/include/ntsc_dmx \
  $(LIB_ZVBI_PATH)/src
endif

LOCAL_MODULE:= tvserver

LOCAL_INIT_RC := tvserver.rc

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)
