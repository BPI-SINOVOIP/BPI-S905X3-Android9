# Copyright (C) 2018 Amlogic Inc
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
# This file is the build configuration for a full Android
# build for Meson reference board.
#

PRODUCT_DIR := m2s_tablet

# Dynamic enable start/stop zygote_secondary in 64bits
# and 32bit system, default closed
#TARGET_DYNAMIC_ZYGOTE_SECONDARY_ENABLE := true

# Inherit from those products. Most specific first.
ifeq ($(ANDROID_BUILD_TYPE), 64)
ifeq ($(TARGET_DYNAMIC_ZYGOTE_SECONDARY_ENABLE), true)
$(call inherit-product, device/bananapi/common/dynamic_zygote_seondary/dynamic_zygote_64_bit.mk)
else
$(call inherit-product, build/target/product/core_64_bit.mk)
endif
endif

$(call inherit-product, device/bananapi/$(PRODUCT_DIR)/vendor_prop.mk)
$(call inherit-product, device/bananapi/common/products/mbox/product_mbox.mk)
$(call inherit-product, device/bananapi/$(PRODUCT_DIR)/device.mk)
$(call inherit-product-if-exists, vendor/google/products/gms.mk)

TARGET_WITH_MEDIA_EXT_LEVEL := 4

#########################################################################
#
#                     media ext
#
#########################################################################
ifeq ($(TARGET_WITH_MEDIA_EXT_LEVEL), 1)
    TARGET_WITH_MEDIA_EXT :=true
    TARGET_WITH_SWCODEC_EXT :=true
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
endif
endif
endif
endif
# m2s_tablet:
PRODUCT_PROPERTY_OVERRIDES += \
        ro.hdmi.device_type=4 \
        persist.sys.hdmi.keep_awake=false

PRODUCT_NAME := m2s_tablet
PRODUCT_DEVICE := m2s_tablet
PRODUCT_BRAND := bananapi
PRODUCT_MODEL := m2s_tablet
PRODUCT_MANUFACTURER := Sinovoip

TARGET_KERNEL_BUILT_FROM_SOURCE := true

PRODUCT_TYPE := mbox

BOARD_AML_TDK_KEY_PATH := device/bananapi/common/tdk_keys/
WITH_LIBPLAYER_MODULE := false

OTA_UP_PART_NUM_CHANGED := false

BOARD_AML_VENDOR_PATH := vendor/amlogic/common/

BOARD_WIDEVINE_TA_PATH := vendor/amlogic/

#AB_OTA_UPDATER :=true
BUILD_WITH_AVB := true

ifeq ($(BUILD_WITH_AVB),true)
#BOARD_AVB_ENABLE := true
BOARD_BUILD_DISABLED_VBMETAIMAGE := true
BOARD_AVB_ALGORITHM := SHA256_RSA2048
BOARD_AVB_KEY_PATH := device/bananapi/common/security/testkey_rsa2048.pem
BOARD_AVB_ROLLBACK_INDEX := 0
endif

BOARD_BUILD_SYSTEM_ROOT_IMAGE := true

ifeq ($(AB_OTA_UPDATER),true)
AB_OTA_PARTITIONS := \
    boot \
    system \
    vendor \
    vbmeta \
    odm \
    dtbo \
    product

TARGET_BOOTLOADER_CONTROL_BLOCK := true
TARGET_NO_RECOVERY := true
BOARD_USES_RECOVERY_AS_BOOT := true
BOARD_USES_SYSTEM_OTHER_ODEX := true

TARGET_PARTITION_DTSI := partition_mbox_ab_P_32.dtsi

ifeq ($(BOARD_BUILD_DISABLED_VBMETAIMAGE), true)
TARGET_FIRMWARE_DTSI := firmware_ab.dtsi
else
TARGET_FIRMWARE_DTSI := firmware_avb_ab.dtsi
endif

else
TARGET_NO_RECOVERY := false

ifeq ($(ANDROID_BUILD_TYPE), 64)
TARGET_PARTITION_DTSI := partition_mbox_normal_P_64.dtsi
else
TARGET_PARTITION_DTSI := partition_mbox_normal_P_32.dtsi
endif

ifneq ($(BUILD_WITH_AVB),true)
TARGET_FIRMWARE_DTSI := firmware_normal.dtsi
else
ifeq ($(BOARD_BUILD_SYSTEM_ROOT_IMAGE), true)
ifeq ($(BOARD_BUILD_DISABLED_VBMETAIMAGE), true)
TARGET_FIRMWARE_DTSI := firmware_system.dtsi
else
TARGET_FIRMWARE_DTSI := firmware_avb_system.dtsi
endif
else
ifeq ($(BOARD_BUILD_DISABLED_VBMETAIMAGE), true)
TARGET_FIRMWARE_DTSI := firmware_normal.dtsi
else
TARGET_FIRMWARE_DTSI := firmware_avb.dtsi
endif
endif
endif

BOARD_CACHEIMAGE_PARTITION_SIZE := 69206016
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 25165824
endif

#########Support compiling out encrypted zip/aml_upgrade_package.img directly
#PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY := true
#PRODUCT_AML_SECURE_BOOT_VERSION3 := true
ifeq ($(PRODUCT_AML_SECURE_BOOT_VERSION3),true)
PRODUCT_AML_SECUREBOOT_RSAKEY_DIR := ./bootloader/uboot-repo/bl33/board/amlogic/bananapi_m2s_v1/aml-key
PRODUCT_AML_SECUREBOOT_AESKEY_DIR := ./bootloader/uboot-repo/bl33/board/amlogic/bananapi_m2s_v1/aml-key
PRODUCT_SBV3_SIGBL_TOOL  := ./bootloader/uboot-repo/fip/stool/amlogic-sign-g12a.sh -s g12b
PRODUCT_SBV3_SIGIMG_TOOL := ./bootloader/uboot-repo/fip/stool/signing-tool-g12a/sign-boot-g12a.sh --sign-kernel -h 2
else
PRODUCT_AML_SECUREBOOT_USERKEY := ./bootloader/uboot-repo/bl33/board/amlogic/bananapi_m2s_v1/aml-user-key.sig
PRODUCT_AML_SECUREBOOT_SIGNTOOL := ./bootloader/uboot-repo/fip/g12b/aml_encrypt_g12b
PRODUCT_AML_SECUREBOOT_SIGNBOOTLOADER := $(PRODUCT_AML_SECUREBOOT_SIGNTOOL) --bootsig \
						--amluserkey $(PRODUCT_AML_SECUREBOOT_USERKEY) \
						--aeskey enable
PRODUCT_AML_SECUREBOOT_SIGNIMAGE := $(PRODUCT_AML_SECUREBOOT_SIGNTOOL) --imgsig \
					--amluserkey $(PRODUCT_AML_SECUREBOOT_USERKEY)
PRODUCT_AML_SECUREBOOT_SIGBIN	:= $(PRODUCT_AML_SECUREBOOT_SIGNTOOL) --binsig \
					--amluserkey $(PRODUCT_AML_SECUREBOOT_USERKEY)
endif# PRODUCT_AML_SECURE_BOOT_VERSION3 := true

########################################################################
#
#                           Kernel Arch
#
########################################################################
ifndef KERNEL_A32_SUPPORT
KERNEL_A32_SUPPORT := true
endif

########################################################################
#
#                           ATV
#
########################################################################
ifeq ($(BOARD_COMPILE_ATV),true)
BOARD_COMPILE_CTS := true
TARGET_BUILD_GOOGLE_ATV:= true
DONT_DEXPREOPT_PREBUILTS:= true
endif
########################################################################

########################################################################
#
#                           CTS
#
########################################################################
ifeq ($(BOARD_COMPILE_CTS),true)
BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 1
BOARD_PLAYREADY_LEVEL := 1
TARGET_BUILD_CTS:= true
TARGET_BUILD_NETFLIX:= true
TARGET_BUILD_NETFLIX_MGKID := true
endif
########################################################################

#########################################################################
#
#                                                Dm-Verity
#
#########################################################################
#TARGET_USE_SECURITY_DM_VERITY_MODE_WITH_TOOL := true
ifeq ($(TARGET_USE_SECURITY_DM_VERITY_MODE_WITH_TOOL), true)
BUILD_WITH_DM_VERITY := true
endif # ifeq ($(TARGET_USE_SECURITY_DM_VERITY_MODE_WITH_TOOL), true)
ifeq ($(BUILD_WITH_DM_VERITY), true)
PRODUCT_PACKAGES += \
	libfs_mgr \
	fs_mgr \
	slideshow
endif

ifeq ($(AB_OTA_UPDATER),true)
PRODUCT_COPY_FILES += \
    device/bananapi/$(PRODUCT_DIR)/fstab.ab.amlogic:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.amlogic
else
PRODUCT_COPY_FILES += \
    device/bananapi/$(PRODUCT_DIR)/fstab.system.amlogic:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.amlogic
endif

#########################################################################
#
#                                                WiFi
#
#########################################################################

WIFI_MODULE := multiwifi
#WIFI_MODULE := BCMWIFI
#WIFI_BUILD_IN := true
include hardware/amlogic/wifi/configs/wifi.mk



#########################################################################
#
#                                                Bluetooth
#
#########################################################################

BOARD_HAVE_BLUETOOTH := true
BLUETOOTH_MODULE := RTKBT
BLUETOOTH_INF := USB
include hardware/amlogic/bluetooth/configs/bluetooth.mk


#########################################################################
#
#                                                ConsumerIr
#
#########################################################################

#PRODUCT_PACKAGES += \
#    consumerir.amlogic \
#    SmartRemote
#PRODUCT_COPY_FILES += \
#    frameworks/native/data/etc/android.hardware.consumerir.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.consumerir.xml


#PRODUCT_PACKAGES += libbt-vendor

ifeq ($(SUPPORT_HDMIIN),true)
PRODUCT_PACKAGES += \
    libhdmiin \
    HdmiIn
endif

#########################################################################
#
#                            Ethernet
#
#########################################################################

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.ethernet.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.ethernet.xml

PRODUCT_PROPERTY_OVERRIDES += \
        ro.net.eth_primary=eth0 \
        ro.net.eth_secondary=eth1

# 0-dhcp, 1-static
PRODUCT_PROPERTY_OVERRIDES += \
        persist.net.eth1.mode=1 \
        persist.net.eth1.staticinfo=172.16.1.1,24,172.16.1.1,114.114.114.114,8.8.8.8 \
        persist.dhcpserver.enable=1 \
        persist.dhcpserver.start=172.16.1.100 \
        persist.dhcpserver.end=172.16.1.150
#########################################################################

# Audio
#
BOARD_ALSA_AUDIO=tiny
include device/bananapi/common/audio.mk

#########################################################################
#
#                                                Camera
#
#########################################################################

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.camera.external.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.external.xml

#########################################################################
#
#                                                GDC
#
#########################################################################
BOARD_GDC_FW_BUILTIN := true
BOARD_GDC_LIB := true

ifeq ($(BOARD_GDC_FW_BUILTIN),true)
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,$(LOCAL_PATH)/gdc,$(TARGET_COPY_OUT_VENDOR)/lib/firmware/gdc)
endif

ifeq ($(BOARD_GDC_LIB),true)
PRODUCT_PACKAGES += libgdc
endif

#########################################################################
#
#                                                PlayReady DRM
#
#########################################################################
#export BOARD_PLAYREADY_LEVEL=3 for PlayReady+NOTVP
#export BOARD_PLAYREADY_LEVEL=1 for PlayReady+OPTEE+TVP
#########################################################################
#
#                                                Verimatrix DRM
##########################################################################
#verimatrix web
BUILD_WITH_VIEWRIGHT_WEB := false
#verimatrix stb
BUILD_WITH_VIEWRIGHT_STB := false
#########################################################################


#DRM Widevine
ifeq ($(BOARD_WIDEVINE_OEMCRYPTO_LEVEL),)
BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 3
endif

ifeq ($(BOARD_WIDEVINE_OEMCRYPTO_LEVEL), 1)
TARGET_USE_OPTEEOS := true
TARGET_ENABLE_TA_SIGN := true
TARGET_USE_HW_KEYMASTER := true
endif

#########################################################################
#
#                                                Miracast Application
##########################################################################
#Miracast Function
BUILD_WITH_MIRACAST := true
ifeq ($(BUILD_WITH_MIRACAST),)
BUILD_WITH_MIRACAST := false
endif

ifeq ($(BUILD_WITH_MIRACAST), true)
BUILD_WITH_MIRACAST := true
ifeq ($(BUILD_WITH_MIRACAST_HDCP),true)
TARGET_USE_OPTEEOS := true
BUILD_WITH_MIRACAST_HDCP := true
endif
endif

$(call inherit-product, device/bananapi/common/media.mk)

#########################################################################
#
#                                                Languages
#
#########################################################################

# For all locales, $(call inherit-product, build/target/product/languages_full.mk)
PRODUCT_LOCALES := en_US en_AU en_IN fr_FR it_IT es_ES et_EE de_DE nl_NL cs_CZ pl_PL ja_JP \
  zh_TW zh_CN zh_HK ru_RU ko_KR nb_NO es_US da_DK el_GR tr_TR pt_PT pt_BR rm_CH sv_SE bg_BG \
  ca_ES en_GB fi_FI hi_IN hr_HR hu_HU in_ID iw_IL lt_LT lv_LV ro_RO sk_SK sl_SI sr_RS uk_UA \
  vi_VN tl_PH ar_EG fa_IR th_TH sw_TZ ms_MY af_ZA zu_ZA am_ET hi_IN en_XA ar_XB fr_CA km_KH \
  lo_LA ne_NP si_LK mn_MN hy_AM az_AZ ka_GE my_MM mr_IN ml_IN is_IS mk_MK ky_KG eu_ES gl_ES \
  bn_BD ta_IN kn_IN te_IN uz_UZ ur_PK kk_KZ

#################################################################################
#
#                                                PPPOE
#
#################################################################################
#ifneq ($(TARGET_BUILD_GOOGLE_ATV), true)
#BUILD_WITH_PPPOE := true
#endif

ifeq ($(BUILD_WITH_PPPOE),true)
PRODUCT_PACKAGES += \
    PPPoE \
    libpppoejni \
    libpppoe \
    pppoe_wrapper \
    pppoe \
    droidlogic.frameworks.pppoe \
    droidlogic.external.pppoe \
    droidlogic.software.pppoe.xml
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.platform.has.pppoe=true
endif

#################################################################################
#
#                                                DEFAULT LOWMEMORYKILLER CONFIG
#
#################################################################################
BUILD_WITH_LOWMEM_COMMON_CONFIG := true

BOARD_USES_USB_PM := true


include device/bananapi/common/software.mk
ifeq ($(TARGET_BUILD_GOOGLE_ATV),true)
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=320
else
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=213
endif


#########################################################################
#
#                                     A/B update
#
#########################################################################
ifeq ($(BUILD_WITH_AVB),true)
PRODUCT_PACKAGES += \
	bootctrl.amlogic \
	libavb_user
endif

ifeq ($(AB_OTA_UPDATER),true)
PRODUCT_PACKAGES += \
    bootctrl.amlogic \
    bootctl

PRODUCT_PACKAGES += \
    update_engine \
    update_engine_client \
    update_verifier \
    delta_generator \
    brillo_update_payload \
    android.hardware.boot@1.0 \
    android.hardware.boot@1.0-impl \
    android.hardware.boot@1.0-service

PRODUCT_PACKAGES += \
    otapreopt_script \
    cppreopts.sh

PRODUCT_PROPERTY_OVERRIDES += \
    ro.cp_system_other_odex=1

AB_OTA_POSTINSTALL_CONFIG += \
    RUN_POSTINSTALL_system=true \
    POSTINSTALL_PATH_system=system/bin/otapreopt_script \
    FILESYSTEM_TYPE_system=ext4 \
    POSTINSTALL_OPTIONAL_system=true
endif

#########################################################################
#
#                                     TB detect
#
#########################################################################
$(call inherit-product, device/bananapi/common/tb_detect.mk)

include device/bananapi/common/gpu/gondul-user-arm64.mk
#####npu ovx service
BOARD_NPU_SERVICE_ENABLE := true
ifeq ($(BOARD_NPU_SERVICE_ENABLE), true)
PRODUCT_CHIP_ID :=PID0x88
PRODUCT_PACKAGES += android.hardware.neuralnetworks@1.1-service-ovx-driver
PRODUCT_PACKAGES += \
        libCLC \
        libGAL \
        libarchmodelSw \
        libNNArchPerf \
        libOpenCL \
        libOpenVX \
        libOpenVXU \
        libovxlib \
        libVSC \
        libnnrt \
        libNNGPUBinary \
        libNNVXCBinary \
        libOvx12VXCBinary
endif
#########################################################################
#
#                                     Auto Patch
#                          must put in the end of mk files
#########################################################################
AUTO_PATCH_SHELL_FILE := vendor/amlogic/common/tools/auto_patch/auto_patch.sh
AUTO_PATCH_AB := vendor/amlogic/common/tools/auto_patch/auto_patch_ab.sh
#HAVE_WRITED_SHELL_FILE := $(shell test -f $(AUTO_PATCH_SHELL_FILE) && echo yes)

ifneq ($(TARGET_BUILD_LIVETV),true)
TARGET_BUILD_LIVETV := false
endif
ifneq ($(TARGET_BUILD_GOOGLE_ATV),true)
TARGET_BUILD_GOOGLE_ATV := false
endif
ifeq ($(HAVE_WRITED_SHELL_FILE),yes)
$(warning $(shell ($(AUTO_PATCH_SHELL_FILE) $(TARGET_BUILD_LIVETV) $(TARGET_BUILD_GOOGLE_ATV))))
ifeq ($(AB_OTA_UPDATER),true)
$(warning $(shell ($(AUTO_PATCH_AB) $(PRODUCT_DIR))))
endif
endif

#########################################################################
#
#                            apps
#
#########################################################################
PRODUCT_PACKAGES += \
    Launcher3

#Add Simple setupwizard to set Settings.Secure.USER_SETUP=1 for notice that user setup complete
PRODUCT_PACKAGES += \
    Provision
#########################################################################
#
#                            factory test
#
#########################################################################
BPI_FACTORY_TEST := false
ifeq ($(BPI_FACTORY_TEST), true)
PRODUCT_PACKAGES += ProductTest
PRODUCT_PACKAGES += DragonAging
PRODUCT_PACKAGES += DragonFire
PRODUCT_PACKAGES += DragonPhone
PRODUCT_PACKAGES += memtester
PRODUCT_PACKAGES += AndroidStressTest

PRODUCT_PROPERTY_OVERRIDES += \
	sys.test.producttest=true
endif
