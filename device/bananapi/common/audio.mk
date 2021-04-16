#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

BUILD_WITH_AMADEC_CTC := false

PRODUCT_PACKAGES += \
    audio_policy.default \
    audio.primary.amlogic \
    audio.hdmi.amlogic \
    audio.r_submix.default \
    acoustics.default \
    audio_firmware \
    libdroidaudiospdif \
    libparameter \
    libamadec_omx_api \
    libfaad    \
    libmad     \
    libamadec_wfd_out \
    libavl \
    libbalance \
    libhpeqwrapper \
    libsrswrapper \
    libtreblebasswrapper \
    libvirtualsurround \
    libms12dapwrapper \
    libgeq \
    libvirtualx \
    libdbx \

ifeq ($(BUILD_WITH_AMADEC_CTC),true)
PRODUCT_PACKAGES += libfaad_sys \
                    libmad_sys
endif
#PRODUCT_COPY_FILES += \
#    $(TARGET_PRODUCT_DIR)/audio_policy.conf:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy.conf \
#    $(TARGET_PRODUCT_DIR)/audio_effects.conf:$(TARGET_COPY_OUT_VENDOR)/etc/audio_effects.conf

#arm audio decoder lib
#soft_adec_libs := $(shell ls hardware/amlogic/LibAudio/amadec/acodec_lib_android_n)
#PRODUCT_COPY_FILES += $(foreach file, $(soft_adec_libs), \
#        hardware/amlogic/LibAudio/amadec/acodec_lib_android_n/$(file):$(TARGET_COPY_OUT_VENDOR)/lib/$(file))
#audio dra decoder lib, now put libstagefrighthw dir for develop omx dra verion in future.
PRODUCT_COPY_FILES += \
    vendor/amlogic/common/prebuilt/libstagefrighthw/lib/libdra.so:$(TARGET_COPY_OUT_VENDOR)/lib/libdra.so
#configurable audio policy
USE_XML_AUDIO_POLICY_CONF := 1

ifeq ($(USE_XML_AUDIO_POLICY_CONF),1)
configurable_audiopolicy_xmls := device/bananapi/common/audio/
PRODUCT_COPY_FILES += \
    $(configurable_audiopolicy_xmls)usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml \
    $(configurable_audiopolicy_xmls)a2dp_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/a2dp_audio_policy_configuration.xml \
    $(configurable_audiopolicy_xmls)r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    $(configurable_audiopolicy_xmls)hearing_aid_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/hearing_aid_audio_policy_configuration.xml \
    $(configurable_audiopolicy_xmls)msd_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/msd_audio_policy_configuration.xml \
    $(configurable_audiopolicy_xmls)default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml

#    $(configurable_audiopolicy_xmls)audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
#    $(configurable_audiopolicy_xmls)audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \

endif

#AQ tuning tool for TV
ifeq ($(BOARD_HAVE_HARDWARE_EQDRC_AUGE),true)
	PRODUCT_COPY_FILES += hardware/amlogic/audio/amlogic_AQ_tools/Amlogic_EQ_Param_Generator:$(TARGET_COPY_OUT_VENDOR)/bin/Amlogic_EQ_Param_Generator
	PRODUCT_COPY_FILES += hardware/amlogic/audio/amlogic_AQ_tools/Amlogic_DRC_Param_Generator:$(TARGET_COPY_OUT_VENDOR)/bin/Amlogic_DRC_Param_Generator
endif

################################################################################## alsa

ifeq ($(BOARD_ALSA_AUDIO),legacy)

PRODUCT_COPY_FILES += \
	$(TARGET_PRODUCT_DIR)/asound.conf:$(TARGET_COPY_OUT_VENDOR)/etc/asound.conf \
	$(TARGET_PRODUCT_DIR)/asound.state:$(TARGET_COPY_OUT_VENDOR)/etc/asound.state

BUILD_WITH_ALSA_UTILS := true

PRODUCT_PACKAGES += \
    alsa.default \
    libasound \
    alsa_aplay \
    alsa_ctl \
    alsa_amixer \
    alsainit-00main \
    alsalib-alsaconf \
    alsalib-pcmdefaultconf \
    alsalib-cardsaliasesconf
endif

################################################################################## tinyalsa

ifeq ($(BOARD_ALSA_AUDIO),tiny)

BUILD_WITH_ALSA_UTILS := false

# Audio
PRODUCT_PACKAGES += \
    audio.usb.default \
    libtinyalsa \
    tinyplay \
    tinycap \
    tinymix \
    audio.usb.amlogic
endif

##################################################################################
ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/mixer_paths.xml),)
    PRODUCT_COPY_FILES += \
        $(TARGET_PRODUCT_DIR)/mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml
else
    ifeq ($(BOARD_AUDIO_CODEC),rt5631)
        PRODUCT_COPY_FILES += \
            hardware/amlogic/audio/rt5631_mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml
    endif

    ifeq ($(BOARD_AUDIO_CODEC),rt5616)
        PRODUCT_COPY_FILES += \
            hardware/amlogic/audio/rt5616_mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml
    endif

    ifeq ($(BOARD_AUDIO_CODEC),wm8960)
        PRODUCT_COPY_FILES += \
            hardware/amlogic/audio/wm8960_mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml
    endif

    ifeq ($(BOARD_AUDIO_CODEC),dummy)
        PRODUCT_COPY_FILES += \
            hardware/amlogic/audio/dummy_mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml
    endif

    ifeq ($(BOARD_AUDIO_CODEC),m8_codec)
        PRODUCT_COPY_FILES += \
            hardware/amlogic/audio/m8codec_mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml
    endif

    ifeq ($(BOARD_AUDIO_CODEC),amlpmu3)
        PRODUCT_COPY_FILES += \
            hardware/amlogic/audio/amlpmu3_mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml
    endif
endif
