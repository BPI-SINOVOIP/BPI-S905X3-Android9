LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

USE_AMADEC_CTC := 1

ifeq ($(USE_AMADEC_CTC), 1)
AMADEC_PATH:=$(TOP)/hardware/amlogic/LibAudio/amadec_ctc/
else
AMADEC_PATH:=$(TOP)/hardware/amlogic/LibAudio/amadec/
endif

LOCAL_SRC_FILES := \
	codec/codec_ctrl.c \
	codec/codec_h_ctrl.c \
	codec/codec_msg.c \
	codec/codec_info.c \
	codec/codec_videoinfo.c \
	audio_ctl/audio_ctrl.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/codec \
	$(LOCAL_PATH)/../amavutils/include \
	$(AMADEC_PATH)/include \
	$(LOCAL_PATH)/audio_ctl

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_C_INCLUDES += \
	$(TOP)/system/core/include
endif

LOCAL_CFLAGS += -Werror -Wformat -Wimplicit-function-declaration
LOCAL_ARM_MODE := arm
ifeq ($(USE_AMADEC_CTC), 1)
LOCAL_STATIC_LIBRARIES := libamadec_ctc_static
else
LOCAL_STATIC_LIBRARIES := libamadec
endif
LOCAL_MODULE:= libamcodec

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -lt 28 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
endif

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	codec/codec_ctrl.c \
	codec/codec_h_ctrl.c \
	codec/codec_msg.c \
	codec/codec_info.c \
	codec/codec_videoinfo.c \
	audio_ctl/audio_ctrl.c


LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/codec \
	$(LOCAL_PATH)/../amavutils/include \
	$(LOCAL_PATH)/audio_ctl \
	$(AMADEC_PATH)/include
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_C_INCLUDES += \
	$(TOP)/system/core/include
endif

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libmedia \
	libz \
	libbinder \
	libdl \
	libcutils \
	libc \
	liblog

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)

ifeq ($(USE_AMADEC_CTC), 1)
LOCAL_SHARED_LIBRARIES += \
	libamavutils_sys \
	libamadec_ctc
else
LOCAL_SHARED_LIBRARIES += \
	libamavutils_sys \
	libamadec_system
endif
else
LOCAL_SHARED_LIBRARIES += \
	libamavutils \
	libamadec
endif

LOCAL_CFLAGS += -Werror -Wformat -Wimplicit-function-declaration
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -lt 28 && echo OK),OK)
LOCAL_VENDOR_MODULE := true
endif
LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libamcodec
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -lt 28 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
endif

include $(BUILD_SHARED_LIBRARY)

###############################################################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	codec/codec_ctrl.c \
	codec/codec_h_ctrl.c \
	codec/codec_msg.c \
	codec/codec_info.c \
	codec/codec_videoinfo.c \
	audio_ctl/dummy_audio_ctrl.c


LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/codec \
	$(LOCAL_PATH)/../amavutils/include \
	$(LOCAL_PATH)/audio_ctl \
	$(AMADEC_PATH)/include
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_C_INCLUDES += \
	$(TOP)/system/core/include
endif

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libz \
	libbinder \
	libdl \
	libcutils \
	libc \
	liblog

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)

ifeq ($(USE_AMADEC_CTC), 1)
LOCAL_SHARED_LIBRARIES += \
	libamavutils_sys
	#libamadec_ctc
else
LOCAL_SHARED_LIBRARIES += \
	libamavutils_sys \
	libamadec_system
endif
else
LOCAL_SHARED_LIBRARIES += \
	libamavutils \
	libamadec
endif

LOCAL_CFLAGS += -Werror -Wformat -Wimplicit-function-declaration \
				-Wno-unused-parameter

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -le 28 && echo OK),OK)
LOCAL_VENDOR_MODULE := true
endif

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libamcodec.vendor
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
