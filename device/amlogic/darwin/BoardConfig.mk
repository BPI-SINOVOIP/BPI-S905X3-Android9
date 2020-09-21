#
# Copyright (C) 2013 The Android Open-Source Project
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

PRODUCT_DIR := darwin

ifneq ($(ANDROID_BUILD_TYPE), 64)
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := cortex-a9
else
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_VARIANT := generic
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_SMP := true

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv7-a-neon
TARGET_2ND_CPU_VARIANT := cortex-a9
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi

TARGET_SUPPORTS_32_BIT_APPS := true
TARGET_SUPPORTS_64_BIT_APPS := true
endif

TARGET_USES_64_BIT_BINDER := true

TARGET_NO_BOOTLOADER := false
TARGET_NO_KERNEL := false
TARGET_NO_RADIOIMAGE := true

TARGET_BOARD_PLATFORM := txlx
TARGET_BOOTLOADER_BOARD_NAME := darwin

# Allow passing `--second` to mkbootimg via 2ndbootloader.
TARGET_BOOTLOADER_IS_2ND := true

# Graphics & Display
USE_OPENGL_RENDERER := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
MAX_VIRTUAL_DISPLAY_DIMENSION := 1920
TARGET_APP_LAYER_USE_CONTINUOUS_BUFFER := true
TARGET_USE_DEFAULT_HDR_PROPERTY := true

#MESONHWC CONFIG
USE_HWC2 := true
HWC_DISPLAY_NUM := 1
HWC_PRIMARY_FRAMEBUFFER_WIDTH := 1920
HWC_PRIMARY_FRAMEBUFFER_HEIGHT := 1080
#HWC_EXTEND_FRAMEBUFFER_WIDTH := 1920
#HWC_EXTEND_FRAMEBUFFER_HEIGHT := 1080
HWC_PRIMARY_CONNECTOR_TYPE := "hdmi"
#HWC_EXTEND_CONNECTOR_TYPE := "panel"
#HWC_ENABLE_HEADLESS_MODE := true
#HWC_ENABLE_SOFTWARE_VSYNC := true
#HWC_ENABLE_PRIMARY_HOTPLUG := true
#HWC_ENABLE_SECURE_LAYER_PROCESS := true
#HWC_DISABLE_CURSOR_PLANE := true

# Camera
USE_CAMERA_STUB := false
BOARD_HAVE_FRONT_CAM := false
BOARD_HAVE_BACK_CAM := false
BOARD_USE_USB_CAMERA := true
IS_CAM_NONBLOCK := true
BOARD_HAVE_FLASHLIGHT := false
BOARD_HAVE_HW_JPEGENC := true

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_USERDATAIMAGE_PARTITION_SIZE := 576716800
BOARD_FLASH_BLOCK_SIZE := 4096

BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_USES_VENDORIMAGE := true
TARGET_COPY_OUT_VENDOR := vendor

ifneq ($(BOARD_BUILD_SYSTEM_ROOT_IMAGE), true)
BOARD_ROOT_EXTRA_FOLDERS += odm
endif

BOARD_ODMIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_USES_ODMIMAGE := true

BOARD_USES_METADATA_PARTITION := true

BOARD_USES_PRODUCTIMAGE := true
BOARD_PRODUCTIMAGE_FILE_SYSTEM_TYPE := ext4
TARGET_COPY_OUT_PRODUCT := product

BOARD_DTBIMAGE_PARTITION_SIZE := 262144
BOARD_BOOTIMAGE_PARTITION_SIZE := 16777216

ifeq ($(ANDROID_BUILD_TYPE), 64)
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1551892480
BOARD_VENDORIMAGE_PARTITION_SIZE := 335544320
BOARD_ODMIMAGE_PARTITION_SIZE := 134217728
BOARD_PRODUCTIMAGE_PARTITION_SIZE := 134217728
else
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1342177280
BOARD_VENDORIMAGE_PARTITION_SIZE := 335544320
BOARD_ODMIMAGE_PARTITION_SIZE := 134217728
BOARD_PRODUCTIMAGE_PARTITION_SIZE := 134217728
endif

# DTBO partition definitions
BOARD_PREBUILT_DTBOIMAGE := out/target/product/$(PRODUCT_DIR)/$(PRODUCT_DIR)-kernel/dtbo.img
BOARD_DTBOIMG_PARTITION_SIZE := 8388608

BOARD_KERNEL_CMDLINE += androidboot.dtbo_idx=0

ifeq ($(BOARD_BUILD_DISABLED_VBMETAIMAGE), true)
ifeq ($(BOARD_BUILD_SYSTEM_ROOT_IMAGE), true)
ifneq ($(AB_OTA_UPDATER),true)
BOARD_KERNEL_CMDLINE += --cmdline "root=/dev/mmcblk0p18"
endif
endif
endif

TARGET_SUPPORT_USB_BURNING_V2 := true
TARGET_AMLOGIC_RES_PACKAGE := device/amlogic/$(PRODUCT_DIR)/logo_img_files

ifeq ($(BOARD_BUILD_SYSTEM_ROOT_IMAGE), true)
TARGET_RECOVERY_FSTAB := device/amlogic/$(PRODUCT_DIR)/recovery/recovery_system.fstab
else
TARGET_RECOVERY_FSTAB := device/amlogic/$(PRODUCT_DIR)/recovery/recovery.fstab
endif

#BOARD_HAL_STATIC_LIBRARIES := libhealthd.mboxdefault

USE_E2FSPROGS := true

BOARD_KERNEL_BASE := 0x0
BOARD_KERNEL_OFFSET := 0x1080000

BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_ALSA_AUDIO := true
TARGET_RELEASETOOLS_EXTENSIONS := device/amlogic/common
TARGET_USE_BLOCK_BASE_UPGRADE := true
TARGET_OTA_UPDATE_DTB := true
#TARGET_RECOVERY_DISABLE_ADB_SIDELOAD := true
#TARGET_OTA_PARTITION_CHANGE := true

TARGET_RECOVERY_PIXEL_FORMAT := BGRA_8888
TARGET_RECOVERY_UI_LIB += libamlogic_ui
TARGET_RECOVERY_UI_LIB += \
    librecovery_amlogic \
    libenv \
    libsystemcontrol_static
ifneq ($(AB_OTA_UPDATER),true)
TARGET_RECOVERY_UPDATER_LIBS := libinstall_amlogic
TARGET_RECOVERY_UPDATER_EXTRA_LIBS += libenv libsystemcontrol_static libsecurity libfdt
endif

include device/amlogic/common/sepolicy.mk
#MALLOC_IMPL := dlmalloc
MALLOC_SVELTE := true

PRODUCT_FULL_TREBLE_OVERRIDE := true
BOARD_PROPERTY_OVERRIDES_SPLIT_ENABLED := true

PRODUCT_SHIPPING_API_LEVEL := 28
TARGET_USES_MKE2FS := true

ifneq ($(TARGET_BUILD_GOOGLE_ATV), true)
DEVICE_MANIFEST_FILE := device/amlogic/common/products/tv/manifest/manifest_aosp.xml
else
DEVICE_MANIFEST_FILE := device/amlogic/common/products/tv/manifest/manifest_gtvs.xml
endif
#DEVICE_MATRIX_FILE   := device/amlogic/common/compatibility_matrix.xml

AUTO_PATCH_AB := device/amlogic/common/products/tv/manifest/ab_update.sh
ifeq ($(AB_OTA_UPDATER),true)
$(shell ($(AUTO_PATCH_AB) $(DEVICE_MANIFEST_FILE)))
endif

BOARD_HAS_ADTV := true
BOARD_VNDK_VERSION := current

BOARD_BOOTIMG_HEADER_VERSION := 1
BOARD_INCLUDE_RECOVERY_DTBO := true
