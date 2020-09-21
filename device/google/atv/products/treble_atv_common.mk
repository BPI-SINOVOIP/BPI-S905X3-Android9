#
# Copyright (C) 2018 The Android Open-Source Project
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

# PRODUCT_PROPERTY_OVERRIDES cannot be used here because sysprops will be at
# /vendor/[build|default].prop when build split is on. In order to have sysprops
# on the generic system image, place them in build/make/target/board/
# treble_system.prop.

# Generic system image inherits from AOSP
$(call inherit-product, device/google/atv/products/atv_base.mk)

# Split selinux policy
PRODUCT_FULL_TREBLE_OVERRIDE := true

# Packages required for ATV GSI
PRODUCT_PACKAGES += \
    FrameworkPackageStubs \
    LatinIMEGoogleTvPrebuilt \
    TvProvision \
    TvSampleLeanbackLauncher

# All VNDK libraries (HAL interfaces, VNDK, VNDK-SP, LL-NDK)
PRODUCT_PACKAGES += vndk_package

# SP-NDK:
PRODUCT_PACKAGES += \
    libvulkan \

# The following policy XML files are used as fallback for
# vendors/devices not using XML to configure audio policy.
PRODUCT_COPY_FILES += \
    frameworks/av/services/audiopolicy/config/audio_policy_configuration_generic.xml:system/etc/audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/primary_audio_policy_configuration.xml:system/etc/primary_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:system/etc/r_submix_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:system/etc/audio_policy_volumes.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:system/etc/default_volume_tables.xml \

# Bluetooth:
#   audio.a2dp.default is a system module. Generic system image includes
#   audio.a2dp.default to support A2DP if board has the capability.
PRODUCT_PACKAGES += \
    audio.a2dp.default

# Net:
#   Vendors can use the platform-provided network configuration utilities (ip,
#   iptable, etc.) to configure the Linux networking stack, but these utilities
#   do not yet include a HIDL interface wrapper. This is a solution on
#   Android O.
PRODUCT_PACKAGES += \
    netutils-wrapper-1.0

# Support for the devices with no VNDK enforcing
PRODUCT_COPY_FILES += \
    build/make/target/product/vndk/init.gsi.rc:system/etc/init/init.gsi.rc \
    build/make/target/product/vndk/init.noenforce.rc:system/etc/init/gsi/init.noenforce.rc

# Name space configuration file for non-enforcing VNDK
PRODUCT_PACKAGES += \
    ld.config.noenforce.txt

# Set current VNDK version for GSI
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.gsi.vndk.version=$(PLATFORM_VNDK_VERSION)

# privapp-permissions whitelisting
PRODUCT_COPY_FILES += \
    vendor/google/data/etc/permissions/privapp-permissions-google.xml:system/etc/permissions/privapp-permissions-google.xml \
    vendor/google/data/etc/permissions/privapp-permissions-atv.xml:system/etc/permissions/privapp-permissions-atv.xml
