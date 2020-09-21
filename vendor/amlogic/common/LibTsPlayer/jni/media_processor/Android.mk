LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

##############################################################
ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \= 19))
ifneq (,$(wildcard vendor/amlogic/frameworks/av/LibPlayer))
LIBPLAYER_PATH:=$(TOP)/vendor/amlogic/frameworks/av/LibPlayer
SUBTITLE_SERVICE_PATH:=$(TOP)/vendor/amlogic/apps/SubTitle
else
LIBPLAYER_PATH := $(TOP)/packages/amlogic/LibPlayer
SUBTITLE_SERVICE_PATH:=$(TOP)/packages/amlogic/SubTitle
endif
else ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 23))
HARDWARE_PATH := $(TOP)/hardware/amlogic
endif

ifeq ($(shell test -e $(LOCAL_PATH)/update_version.sh && echo 1),1)
    $(shell (cd $(LOCAL_PATH) && sh ./update_version.sh>/dev/null 2>&1))
else
    $(error update_version.sh not found!)
endif

##############################################################
CTC_SRC_FILES :=	\
	CTC_MediaProcessor.cpp \
	CTC_MediaProcessor_ZTE.cpp \
	CTC_Utils.cpp \
	CTsPlayer.cpp \
	ITsPlayer.cpp \
	legacy/legacy_CTC_MediaProcessor.cpp \
	legacy/legacy_CTC_MediaControl.cpp

CTC_SRC_FILES_19 := \
	subtitleservice.cpp \
	CTsOmxPlayer.cpp \

CTC_SRC_FILES_28 := \
	MemoryLeakTrackUtilTmp.cpp \
	subtitleservice.cpp \

CTC_C_INCLUDES := \
	$(JNI_H_INCLUDE)/ \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../compat \
	$(TOP)/frameworks/av/ \
	$(TOP)/frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/include/media/openmax \

CTC_C_INCLUDES_19 := $(TOP)/external/stlport/stlport \
	$(LIBPLAYER_PATH)/amplayer/player/include \
	$(LIBPLAYER_PATH)/amplayer/control/include \
	$(LIBPLAYER_PATH)/amffmpeg \
	$(LIBPLAYER_PATH)/amcodec/include \
	$(LIBPLAYER_PATH)/amcodec/amsub_ctl \
	$(LIBPLAYER_PATH)/amadec/include \
	$(LIBPLAYER_PATH)/amavutils/include \
	$(LIBPLAYER_PATH)/amsubdec \
	$(SUBTITLE_SERVICE_PATH)/service \

CTC_C_INCLUDES_28 := \
	$(HARDWARE_PATH)/LibAudio/amadec/include \
	$(HARDWARE_PATH)/media/amcodec/include \
	$(HARDWARE_PATH)/media/amvdec/include \
	$(HARDWARE_PATH)/media/amavutils/include \
	$(TOP)/frameworks/native/libs/nativewindow/include \
	$(HARDWARE_PATH)/gralloc/amlogic \
	$(TOP)/vendor/amlogic/common/external/dvb/include/am_adp \
	$(TOP)/vendor/amlogic/common/external/dvb/android/ndk/include \
	$(TOP)/vendor/amlogic/common/frameworks/av/mediaextconfig/include \
	$(TOP)/vendor/verimatrix/iptvclient/include \
	$(TOP)/vendor/amlogic/common/external/ffmpeg \
	$(TOP)/vendor/amlogic/common/frameworks/services/subtiltleserver/subtitleServerHidlClient

CTC_CFLAGS := -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

CTC_SHARED_LIBRARIES := libutils libmedia libz libbinder \
	liblog libcutils libdl libgui libui \
	libstagefright libstagefright_foundation

CTC_SHARED_LIBRARIES_19 := libstlport libamcodec libamadec \
	libamsubdec libamavutils libamFFExtractor libFFExtractor \
	libamplayer libsubtitleservice

CTC_SHARED_LIBRARIES_28 := libamavutils libamcodec libaudioclient \
	libamgralloc_ext@2 libhidltransport libbase \
	libamffmpeg \
	vendor.amlogic.hardware.subtitleserver@1.0 libhidltransport libbase libsubtitlebinder

##############################################################
LOCAL_SRC_FILES := $(CTC_SRC_FILES) $(CTC_SRC_FILES_$(PLATFORM_SDK_VERSION))
LOCAL_C_INCLUDES := $(CTC_C_INCLUDES) $(CTC_C_INCLUDES_$(PLATFORM_SDK_VERSION))
LOCAL_CFLAGS := $(CTC_CFLAGS) $(CTC_CFLAGS_$(PLATFORM_SDK_VERSION))
LOCAL_SHARED_LIBRARIES := $(CTC_SHARED_LIBRARIES) $(CTC_SHARED_LIBRARIES_$(PLATFORM_SDK_VERSION))
LOCAL_ARM_MODE := arm
LOCAL_MODULE    := libCTC_MediaProcessor
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include

##############################################################
ifeq ($(PLATFORM_SDK_VERSION), 19))

ifeq ($(IPTV_ZTE_SUPPORT),true)
LOCAL_CFLAGS += -IPTV_ZTE_SUPPORT
endif

ifeq ($(TELECOM_VFORMAT_SUPPORT),true)
LOCAL_CFLAGS += -DTELECOM_VFORMAT_SUPPORT
endif

ifeq ($(TELECOM_QOS_SUPPORT),true)
LOCAL_CFLAGS += -DTELECOM_QOS_SUPPORT
endif

ifeq ($(NEED_AML_CTC_MIDDLE),true)
LOCAL_SHARED_LIBRARIES += libCTC_AmMediaControl libCTC_AmMediaProcessor libffmpeg30
endif

ifeq ($(TARGET_USE_OPTEEOS),true)
LOCAL_SHARED_LIBRARIES += libtelecom_iptv
LOCAL_CFLAGS += -DUSE_OPTEEOS
endif

endif

include $(BUILD_SHARED_LIBRARY)

