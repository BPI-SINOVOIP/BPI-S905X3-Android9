LOCAL_PATH:= $(call my-dir)
include $(TOP)/hardware/amlogic/media/media_base_config.mk
AMAVUTILS_INCLUDE=$(AMAVUTILS_PATH)/include/
ifeq ($(BUILD_WITH_BOOT_PLAYER),true)
include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -fPIC -D_POSIX_SOURCE


ifneq (0, $(shell expr $(PLATFORM_VERSION) \< 5.0))
ifneq (0, $(shell expr $(PLATFORM_VERSION) \>= 5.0))
ALSA_LIB_DIR=$(BOARD_AML_VENDOR_PATH)/external/alsa-lib/
else
ALSA_LIB_DIR=external/alsa-lib/
endif
LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/include \
    $(AMAVUTILS_INCLUDE) \
    $(TOP)/$(ALSA_LIB_DIR)/include


ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.1.0))
    LOCAL_CFLAGS += -D_VERSION_JB
else
    ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.0.0))
        LOCAL_CFLAGS += -D_VERSION_ICS
    endif
endif

LOCAL_CFLAGS += -DALSA_OUT
#ifdef DOLBY_UDC
LOCAL_CFLAGS+=-DDOLBY_USE_ARMDEC
#endif
LOCAL_SHARED_LIBRARIES += libasound audio.primary.amlogic

LOCAL_SRC_FILES := \
           adec-external-ctrl.c adec-internal-mgt.c adec-ffmpeg-mgt.c adec-message.c adec-pts-mgt.c feeder.c adec_write.c adec_read.c\
           audio_out/alsa-out.c audio_out/aml_resample.c audiodsp_update_format.c spdif_api.c pcmenc_api.c \
           dts_transenc_api.c dts_enc.c adec_omx_brige.c

LOCAL_MODULE := libamadec_alsa

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)

endif
endif

include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -fPIC -D_POSIX_SOURCE
#ifdef DOLBY_UDC
    LOCAL_CFLAGS+=-DDOLBY_USE_ARMDEC
#endif

ifdef DOLBY_DS1_UDC
  LOCAL_CFLAGS += -DDOLBY_DS1_UDC
endif

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/include \
	$(AMAVUTILS_INCLUDE) \
    system/core/base/include \
    frameworks/av/include \
    system/media/audio/include \
    hardware/amlogic/audio/audio_hal \
    hardware/amlogic//audio/utils/include/

# PLATFORM_SDK_VERSION:
# 4.4 = 19
# 4.3 = 18
# 4.2 = 17
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

ifneq (0, $(shell expr $(PLATFORM_VERSION) \>= 4.3))
    LOCAL_CFLAGS += -DANDROID_VERSION_JBMR2_UP=1
    ifneq ($(TARGET_BOARD_PLATFORM),meson6)
        LOCAL_CFLAGS += -DUSE_ARM_AUDIO_DEC
    endif
endif

ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.1.0))
    LOCAL_CFLAGS += -D_VERSION_JB
else
    ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.0.0))
        LOCAL_CFLAGS += -D_VERSION_ICS
    endif
endif

LOCAL_SRC_FILES := \
           adec-external-ctrl.c adec-internal-mgt.c adec-ffmpeg-mgt.c adec-message.c adec-pts-mgt.c adec_write.c adec_read.c\
           audio_out/dtv_patch_out.c audio_out/aml_resample.c audiodsp_update_format.c
LOCAL_MODULE := libamadec

LOCAL_ARM_MODE := arm


include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -fPIC -D_POSIX_SOURCE
#ifdef DOLBY_UDC
    LOCAL_CFLAGS+=-DDOLBY_USE_ARMDEC
#endif



LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/include \
    $(AMAVUTILS_INCLUDE) \
    frameworks/av/include \
    system/media/audio/include \
    system/libhidl/transport/token/1.0/utils/include \
    hardware/amlogic/audio/audio_hal \
    hardware/amlogic/audio/utils/include/


LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_CFLAGS += -DUSE_ARM_AUDIO_DEC
ifneq (0, $(shell expr $(PLATFORM_VERSION) \>= 4.3))
    LOCAL_CFLAGS += -DANDROID_VERSION_JBMR2_UP=1
endif

ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.1.0))
    LOCAL_CFLAGS += -D_VERSION_JB
else
    ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.0.0))
        LOCAL_CFLAGS += -D_VERSION_ICS
    endif
endif

LOCAL_SRC_FILES := \
           adec-external-ctrl.c adec-internal-mgt.c adec-ffmpeg-mgt.c adec-message.c adec-pts-mgt.c adec_write.c adec_read.c\
           audio_out/dtv_patch_out.c  \
           adec_omx_brige.c

LOCAL_MODULE := libamadec

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_ARM_MODE := arm
##################################################
#$(shell cp $(LOCAL_PATH)/acodec_lib/*.so $(TARGET_OUT)/lib)
###################################################
LOCAL_SHARED_LIBRARIES += libutils libz libbinder libdl libcutils libc libamavutils liblog libdtvad libamaudioutils

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

#########################################################################################################################################################
include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -fPIC -D_POSIX_SOURCE
#ifdef DOLBY_UDC
    LOCAL_CFLAGS+=-DDOLBY_USE_ARMDEC
#endif


 
LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/include \
    $(AMAVUTILS_INCLUDE) \
    frameworks/av/include \
    system/media/audio/include \
    system/libhidl/transport/token/1.0/utils/include


LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_CFLAGS += -DUSE_ARM_AUDIO_DEC
ifneq (0, $(shell expr $(PLATFORM_VERSION) \>= 4.3))
    LOCAL_CFLAGS += -DANDROID_VERSION_JBMR2_UP=1
endif

ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.1.0))
    LOCAL_CFLAGS += -D_VERSION_JB
else
    ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.0.0))
        LOCAL_CFLAGS += -D_VERSION_ICS
    endif
endif

LOCAL_SRC_FILES := \
           adec-external-ctrl.c adec-internal-mgt.c adec-ffmpeg-mgt.c adec-message.c adec-pts-mgt.c feeder.c adec_write.c adec_read.c\
           dsp/audiodsp-ctl.c audio_out/android-out.cpp audio_out/aml_resample.c audiodsp_update_format.c \
           adec_omx_brige.c adec-wfd.c

LOCAL_MODULE := libamadec_system
LOCAL_CFLAGS+=-DUSE_AOUT_IN_ADEC
LOCAL_ARM_MODE := arm
##################################################
#$(shell cp $(LOCAL_PATH)/acodec_lib/*.so $(TARGET_OUT)/lib)
###################################################
LOCAL_SHARED_LIBRARIES += libutils libz libbinder libdl libcutils libc libamavutils_sys liblog libaudioclient

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)


#
# audio_firmware module
#   includes all audio firmware files, which are modules themselves.
#
ifeq (0, $(shell expr $(PLATFORM_VERSION) \>= 6.0))
include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM),meson6)
    audio_firmware_dir := firmware-m6
else ifeq ($(TARGET_BOARD_PLATFORM),meson8)
    audio_firmware_dir := firmware-m8
else
    audio_firmware_dir := firmware
endif

# generate md5 checksum files
$(shell cd $(LOCAL_PATH)/$(audio_firmware_dir) && { \
for f in *.bin; do \
  md5sum "$$f" > "$$f".checksum; \
done;})

# gather list of relative filenames
audio_firmware_files := $(wildcard $(LOCAL_PATH)/$(audio_firmware_dir)/*.bin)
audio_firmware_files += $(wildcard $(LOCAL_PATH)/$(audio_firmware_dir)/*.checksum)
audio_firmware_files := $(patsubst $(LOCAL_PATH)/%,%,$(audio_firmware_files))

# define function to create a module for each file
# $(1): filename
define _add-audio-firmware-module
    include $$(CLEAR_VARS)
    LOCAL_MODULE := audio-firmware_$(notdir $(1))
    LOCAL_MODULE_STEM := $(notdir $(1))
    _audio_firmware_modules += $$(LOCAL_MODULE)
    LOCAL_SRC_FILES := $1
    LOCAL_MODULE_TAGS := optional
    LOCAL_MODULE_CLASS := ETC
    LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/etc/firmware
    include $$(BUILD_PREBUILT)
endef

# create modules, one for each file
_audio_firmware_modules :=
_audio_firmware :=
$(foreach _firmware, $(audio_firmware_files), \
  $(eval $(call _add-audio-firmware-module,$(_firmware))))

#LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := audio_firmware
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional

LOCAL_REQUIRED_MODULES := $(_audio_firmware_modules)

include $(BUILD_PHONY_PACKAGE)

_audio_firmware_modules :=
_audio_firmware :=
endif
