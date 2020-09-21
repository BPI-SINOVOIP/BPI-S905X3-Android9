# Copyright (c) 2014 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.
#
# Description: makefile

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

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


LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  tv/CAutoPQparam.cpp \
  tv/CBootvideoStatusDetect.cpp \
  tv/CTvEv.cpp \
  tv/CTvSubtitle.cpp \
  tv/CTvTime.cpp \
  tv/CTv.cpp \
  tv/CTvBooking.cpp \
  tv/CTvVchipCheck.cpp \
  tv/CTvScreenCapture.cpp \
  tv/CAv.cpp \
  tv/CTvDmx.cpp \
  tv/CTvFactory.cpp \
  tvin/CTvin.cpp \
  tvin/CDevicesPollStatusDetect.cpp \
  tvin/CHDMIRxManager.cpp \
  tvdb/CTvDimension.cpp \
  tv/CTvPlayer.cpp \
  tvdb/CTvChannel.cpp \
  tvdb/CTvEvent.cpp \
  tvdb/CTvGroup.cpp \
  tvdb/CTvProgram.cpp \
  tvdb/CTvRegion.cpp \
  tvdb/CTvDatabase.cpp \
  tv/CTvScanner.cpp \
  tv/CFrontEnd.cpp \
  tv/CTvEpg.cpp \
  tv/CTvRrt.cpp \
  tv/CTvEas.cpp \
  tv/CTvRecord.cpp \
  vpp/CVpp.cpp \
  tvsetting/CTvSetting.cpp  \
  tvsetting/TvKeyData.cpp \
  version/version.cpp \
  gpio/CTvGpio.cpp

LOCAL_SHARED_LIBRARIES := \
  libui \
  libutils \
  libcutils \
  libnetutils \
  libsqlite \
  libtinyxml \
  liblog \
  libbinder

LOCAL_SHARED_LIBRARIES += \
  libtvbinder \
  libsystemcontrolservice

#DVB define
ifeq ($(BOARD_HAS_ADTV),true)
LOCAL_CFLAGS += -DSUPPORT_ADTV

LOCAL_SHARED_LIBRARIES += \
  libzvbi \
  libntsc_decode \
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

LOCAL_STATIC_LIBRARIES += \
  libz \
  libtv_utils \
  libjsoncpp

LOCAL_C_INCLUDES += \
  $(LIB_TV_UTILS)/include

LOCAL_CFLAGS += \
  -fPIC -fsigned-char -D_POSIX_SOURCE \
  -DALSA_CONFIG_DIR=\"/system/usr/share/alsa\" \
  -DALSA_PLUGIN_DIR=\"/system/usr/lib/alsa-lib\" \
  -DALSA_DEVICE_DIRECTORY=\"/dev/snd/\"

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

ifeq ($(SOURCE_DEDTECT_ON),true)
  LOCAL_CFLAGS += -DSOURCE_DETECT_ENABLE
endif

ifeq ($(TARGET_SIMULATOR),true)
  LOCAL_CFLAGS += -DSINGLE_PROCESS
endif

LOCAL_C_INCLUDES += \
  external/tinyxml \
  $(LIB_TV_BINDER)/include \
  $(LIB_SQLITE_PATH)/dist \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol/PQ/include

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

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/tvdb \
  $(LOCAL_PATH)/tv \
  $(LOCAL_PATH)/gpio \
  external/jsoncpp/include \
  frameworks/av/include \
  hardware/libhardware_legacy/include


LOCAL_LDLIBS  += -L$(SYSROOT)/usr/lib -llog

LOCAL_PRELINK_MODULE := false

# version
ifeq ($(strip $(BOARD_TVAPI_NO_VERSION)),)
  $(shell cd $(LOCAL_PATH);touch version/version.cpp)
  LIBTVSERVICE_GIT_VERSION="$(shell cd $(LOCAL_PATH);git log | grep commit -m 1 | cut -d' ' -f 2)"
  LIBTVSERVICE_GIT_UNCOMMIT_FILE_NUM=$(shell cd $(LOCAL_PATH);git diff | grep +++ -c)
  LIBTVSERVICE_GIT_BRANCH="$(shell cd $(LOCAL_PATH);git branch | grep \* -m 1)"
  LIBTVSERVICE_LAST_CHANGED="$(shell cd $(LOCAL_PATH);git log | grep Date -m 1)"
  LIBTVSERVICE_BUILD_TIME=" $(shell date)"
  LIBTVSERVICE_BUILD_NAME=" $(shell echo ${LOGNAME})"

  LOCAL_CFLAGS+=-DHAVE_VERSION_INFO
  LOCAL_CFLAGS+=-DLIBTVSERVICE_GIT_VERSION=\"${LIBTVSERVICE_GIT_VERSION}${LIBTVSERVICE_GIT_DIRTY}\"
  LOCAL_CFLAGS+=-DLIBTVSERVICE_GIT_UNCOMMIT_FILE_NUM=${LIBTVSERVICE_GIT_UNCOMMIT_FILE_NUM}
  LOCAL_CFLAGS+=-DLIBTVSERVICE_GIT_BRANCH=\"${LIBTVSERVICE_GIT_BRANCH}\"
  LOCAL_CFLAGS+=-DLIBTVSERVICE_LAST_CHANGED=\"${LIBTVSERVICE_LAST_CHANGED}\"
  LOCAL_CFLAGS+=-DLIBTVSERVICE_BUILD_TIME=\"${LIBTVSERVICE_BUILD_TIME}\"
  LOCAL_CFLAGS+=-DLIBTVSERVICE_BUILD_NAME=\"${LIBTVSERVICE_BUILD_NAME}\"
  LOCAL_CFLAGS+=-DTVAPI_BOARD_VERSION=\"$(TVAPI_TARGET_BOARD_VERSION)\"
endif

LOCAL_CFLAGS += -DTARGET_BOARD_$(strip $(TVAPI_TARGET_BOARD_VERSION))
LOCAL_MODULE:= libtv
LOCAL_LDFLAGS := -shared

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
