
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
#media related config for amlogic &
#some dynamic shared libraries
#


#for amlogicplayer& liblayer related.
#TARGET_WITH_AMLOGIC_EXTRATORS :=true
#TARGET_WITH_AMLOGIC_SCREAN_MEDIASOURCE :=true
#TARGET_WITH_AMLOGIC_RETRIEVER :=true
#TARGET_WITH_AMLOGIC_PLAYERS :=true
#move TARGET_WITH_MEDIA_EXT_LEVEL to platform mk
#TARGET_WITH_MEDIA_EXT_LEVEL := 3
#set on some prducts,used libplayer.
BUILD_WITH_BOOT_PLAYER :=true
BUILD_WITH_ES_PLAYER := true
BUILD_WITH_CTC_MEDIAPROCESSOR := false

#########################################################################
#
#                     media ext
#
#########################################################################
ifeq ($(TARGET_WITH_MEDIA_EXT_LEVEL), 1)
    TARGET_WITH_MEDIA_EXT :=true
    TARGET_WITH_SWCODEC_EXT := true
else
ifeq ($(TARGET_WITH_MEDIA_EXT_LEVEL), 2)
    TARGET_WITH_MEDIA_EXT :=true
    TARGET_WITH_CODEC_EXT := true
else
ifeq ($(TARGET_WITH_MEDIA_EXT_LEVEL), 3)
    TARGET_WITH_MEDIA_EXT :=true
    TARGET_WITH_SWCODEC_EXT := true
    TARGET_WITH_CODEC_EXT := true
else
ifeq ($(TARGET_WITH_MEDIA_EXT_LEVEL), 4)
    TARGET_WITH_MEDIA_EXT :=true
    TARGET_WITH_SWCODEC_EXT := true
    TARGET_WITH_CODEC_EXT := true
    TARGET_WITH_PLAYERS_EXT := true
#add our private retriever extension
#only can open for customer do no use cts
#defult do not open it
    #TARGET_WITH_RETRIEVER_EXT := true
endif
endif
endif
endif
ifeq ($(TARGET_WITH_MEDIA_EXT), true)
PRODUCT_PACKAGES += \
    libammediaext \
    libammediaext.vendor \
    libamffmpeg \
    libamffmpeg.vendor \
    libamextractor \
    libamffmpegadapter \
    libamffmpegcodec \

endif

#soft codec related.
#
ifeq ($(TARGET_WITH_SWCODEC_EXT), true)
PRODUCT_PACKAGES += \
    libOmxCoreSw \
    libstagefright_soft_amsoftdec

endif

#codec ext related.
#
ifeq ($(TARGET_WITH_CODEC_EXT), true)
PRODUCT_PACKAGES += \
   libavenhancements

ifeq (,$(wildcard $(BOARD_AML_VENDOR_PATH)/frameworks/av/AmFFmpegAdapter))
ifeq (,$(wildcard $(BOARD_AML_VENDOR_PATH)/AmFFmpegAdapter))
PRODUCT_COPY_FILES += \
   $(BOARD_AML_VENDOR_PATH)/prebuilt/libmedia/libavenhancements/vendor/lib/libavenhancements.so:$(TARGET_COPY_OUT_VENDOR)/lib/libavenhancements.so \

endif
endif

endif

#codec ext related.
#
ifeq ($(TARGET_WITH_PLAYERS_EXT), true)
BOARD_USE_CUSTOM_MEDIASERVEREXTENSIONS:=true
TARGET_WITH_AMNUPLAYER :=true

endif

ifeq ($(TARGET_WITH_RETRIEVER_EXT), true)
PRODUCT_PACKAGES += \
    libmetadataretriever_ext
endif

#########################################################################
#
#                                                PlayReady DRM
#
#########################################################################
ifeq ($(BOARD_PLAYREADY_LEVEL),1)
    BUILD_WITH_PLAYREADY_DRM := true
    BOARD_PLAYREADY_TVP := true
    TARGET_USE_OPTEEOS := true
else
ifeq ($(BOARD_PLAYREADY_LEVEL), 3)
    BUILD_WITH_PLAYREADY_DRM := true
endif
endif

ifeq ($(BUILD_WITH_PLAYREADY_DRM),true)

PRODUCT_PACKAGES += libplayreadymediadrmplugin \
  libplayready \
  9a04f079-9840-4286-ab92e65be0885f95

PRODUCT_COPY_FILES += \
    $(BOARD_AML_VENDOR_PATH)/prebuilt/libmediadrm/playready/keycert/zgpriv.dat:$(TARGET_COPY_OUT_VENDOR)/etc/drm/playready/zgpriv.dat \
    $(BOARD_AML_VENDOR_PATH)/prebuilt/libmediadrm/playready/keycert/bgroupcert.dat:$(TARGET_COPY_OUT_VENDOR)/etc/drm/playready/bgroupcert.dat \
    $(BOARD_AML_VENDOR_PATH)/prebuilt/libmediadrm/playready/keycert/zgpriv_protected.dat:$(TARGET_COPY_OUT_VENDOR)/etc/drm/playready/zgpriv_protected.dat

endif

#########################################################################
#
#                                     Verimatrix ViewRight Web
#
#########################################################################
ifeq ($(BUILD_WITH_VIEWRIGHT_WEB),true)

PRODUCT_PACKAGES += libVCASCommunication \

endif

#########################################################################
#
#                                     Verimatrix ViewRight Stb
#
#########################################################################
ifeq ($(BUILD_WITH_VIEWRIGHT_STB),true)

PRODUCT_PACKAGES += libvm_mod \


endif



PRODUCT_PACKAGES += ca-certificates.crt \
    libstagefright_wfd_sink




PRODUCT_PACKAGES += \
    libstagefright_soft_aacdec \
    libstagefright_soft_aacenc \
    libstagefright_soft_amrdec \
    libstagefright_soft_amrnbenc \
    libstagefright_soft_amrwbenc \
    libstagefright_soft_flacenc \
    libstagefright_soft_g711dec \
    libstagefright_soft_mp3dec \
    libstagefright_soft_mp2dec \
    libstagefright_soft_vorbisdec \
    libstagefright_soft_rawdec \
    libstagefright_soft_adpcmdec \
    libstagefright_soft_adifdec \
    libstagefright_soft_latmdec \
    libstagefright_soft_adtsdec \
    libstagefright_soft_alacdec \
    libstagefright_soft_dtshd \
    libstagefright_soft_apedec   \
    libstagefright_soft_wmaprodec \
    libstagefright_soft_wmadec    \
    libstagefright_soft_ddpdcv \

ifeq ($(BUILD_WITH_SOFT_AV1_DECODER), true)
PRODUCT_PACKAGES += \
    libstagefright_soft_av1dec
endif
#for Netflix MGKID
ifeq ($(TARGET_BUILD_NETFLIX_MGKID),true)

PRODUCT_PACKAGES += libnetflixplugin \
  00d1ca22-1764-4e35-90aa-5b8c12630764

endif

#for drm widevine.
PRODUCT_PROPERTY_OVERRIDES += drm.service.enable=true
ifeq ($(BOARD_WIDEVINE_OEMCRYPTO_LEVEL),1)
    TARGET_USE_SECUREOS := true
    CONFIG_SECURE_OS_BDK := true
endif

ifeq ($(TARGET_USE_OPTEEOS), true)
    BOARD_OMX_WITH_OPTEE_TVP := true
    BUILD_WITH_TEEVIDEOFIRM_LOAD :=true
else
ifeq ($(TARGET_USE_SECUREOS), true)
    BOARD_OMX_WITH_TVP := true
endif
endif

PRODUCT_PACKAGES += android.hardware.drm@1.1-service.widevine \
    libwvhidl \
    liboemcrypto \
    e043cde0-61d0-11e5-9c260002a5d5c51b \
    secmem_test \
    oemcrypto_test_aml

ifeq ($(TARGET_WITH_AMLOGIC_PLAYERS), true)
##player related
BUILD_WITH_AMLOGIC_PLAYER := true

PRODUCT_PACKAGES += libmedia_amlogic \
    librtmp \
    libmms_mod \
	libcurl_mod \
    libvhls_mod \
    libprhls_mod.so \
    libdash_mod.so  \
    libbluray.so \
    libbluray_mod.so \

#audio
PRODUCT_PACKAGES += libamadec_omx_api \
    libfaad    \
    libmad     \
    libamadec_wfd_out
else
#no libplayer but have amnuplayer
ifeq ($(TARGET_WITH_AMNUPLAYER), true)
PRODUCT_PACKAGES += libamnuplayer
endif #amnuplayer

endif

ifeq ($(TARGET_WITH_AMLOGIC_RETRIEVER), true)
#retriever
PRODUCT_PACKAGES += libamlogic_metadata_retriever
endif

ifeq ($(TARGET_WITH_AMLOGIC_SCREAN_MEDIASOURCE), true)
#for screensource
PRODUCT_PACKAGES += libstagefright_screenmediasource
endif


ifeq ($(TARGET_WITH_AMLOGIC_EXTRATORS), true)
#for ffmpeg extrator
PRODUCT_PACKAGES += libamffmpegadapter
#for other extrators
PRODUCT_PACKAGES += libstagefright_extrator
endif

ifeq ($(BUILD_WITH_BOOT_PLAYER),true)
PRODUCT_PACKAGES += bootplayer \
    libasound \
    alsalib-alsaconf \
    alsalib-pcmdefaultconf \
    alsalib-cardsaliasesconf

endif

ifeq ($(BUILD_WITH_ES_PLAYER),true)
PRODUCT_PACKAGES += esplayer
endif

ifeq ($(BUILD_WITH_TEEVIDEOFIRM_LOAD),true)
PRODUCT_PACKAGES += \
    libtee_load_video_fw \
    tee_preload_fw \
    526fc4fc-7ee6-4a12-96e3-83da9565bce8
endif

ifeq ($(BOARD_BUILD_VMX_DRM),true)
#for drmplayer vmx
PRODUCT_PACKAGES += libam_adp_adec \
      libamadec_system \
      libamavutils_sys \
      libfaad_sys \
      libmad_sys \
      libteec_sys \
      libDrmPlayer \
      libdrmp \
      libdec_ca \
      41fe9859-71e4-4bf4-bbaad71435b127ae \
      libdec_ca_vmx_iptv \
      libdec_ca_vmx_web
endif

ifeq ($(BUILD_WITH_WIDEVINECAS),true)
#for drmplayer wvcas
PRODUCT_PACKAGES += libam_adp_adec \
      libamadec_system \
      libamavutils_sys \
      libfaad_sys \
      libmad_sys \
      libteec_sys \
      libDrmPlayer \
      libdrmp \
      libdec_ca \
      41fe9859-71e4-4bf4-bbaad71435b127ae \
      libdec_ca_wvcas
endif

#BOARD_SECCOMP_POLICY := device/bananapi/common/seccomp
PRODUCT_COPY_FILES += \
    device/bananapi/common/seccomp/mediaextractor.policy:$(TARGET_COPY_OUT_VENDOR)/etc/seccomp_policy/mediaextractor.policy \
    device/bananapi/common/seccomp/mediacodec.policy:$(TARGET_COPY_OUT_VENDOR)/etc/seccomp_policy/mediacodec.policy


BOARD_AML_MEDIAHAL_PATH := hardware/amlogic/media/
BOARD_AML_LIBAUDIO_PATH := hardware/amlogic/LibAudio/
BOARD_AML_HARDWARE_PATH := hardware/amlogic/
AMLOGIC_FRAMEWORKS_AV_CONFIG_MK := $(BOARD_AML_VENDOR_PATH)/frameworks/av/mediaextconfig/config.mk
BOARD_AML_MEDIA_HAL_CONFIG := $(BOARD_AML_MEDIAHAL_PATH)/media_base_config.mk

# for media modules
PRODUCT_COPY_FILES += \
	device/bananapi/common/init.amlogic.media.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.media.rc


#########################################################################
#
#                     CTC_MediaProcessor
#
#########################################################################
ifeq ($(BUILD_WITH_CTC_MEDIAPROCESSOR),true)
PRODUCT_PACKAGES += \
    libffmpeg40 \
    libFFExtractor \
    libamFFExtractor \
    libCTC_MediaProcessor \
    libCTC_MediaProcessorjni \
    libCTC_AmlPlayer \
    mediaProcessorDemo \
    libminiframework \
    libliveplayer \
    libAmIptvMedia
endif


