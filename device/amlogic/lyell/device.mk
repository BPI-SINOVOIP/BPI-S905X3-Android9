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

PRODUCT_COPY_FILES += \
    device/amlogic/common/products/tv/init.amlogic.system.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.rc \
    device/amlogic/$(PRODUCT_DIR)/init.amlogic.usb.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.usb.rc \
    device/amlogic/$(PRODUCT_DIR)/init.amlogic.board.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.board.rc

ifeq ($(AB_OTA_UPDATER),true)
PRODUCT_COPY_FILES += \
    device/amlogic/common/cppreopts_amlogic.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/cppreopts_amlogic.rc
endif

PRODUCT_COPY_FILES += device/amlogic/common/products/tv/ueventd.amlogic.rc:$(TARGET_COPY_OUT_VENDOR)/ueventd.rc

# DRM HAL
PRODUCT_PACKAGES += \
    android.hardware.drm@1.0-impl:32 \
    android.hardware.drm@1.0-service \
    android.hardware.drm@1.1-service.widevine \
    android.hardware.drm@1.1-service.clearkey \
    move_widevine_data.sh

PRODUCT_COPY_FILES += \
    device/amlogic/$(PRODUCT_DIR)/files/media_profiles.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_profiles.xml \
    device/amlogic/$(PRODUCT_DIR)/files/media_profiles_V1_0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_profiles_V1_0.xml \
    device/amlogic/$(PRODUCT_DIR)/files/media_codecs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs.xml \
    device/amlogic/$(PRODUCT_DIR)/files/media_codecs_performance.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_performance.xml \
    device/amlogic/$(PRODUCT_DIR)/files/mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml \
    device/amlogic/$(PRODUCT_DIR)/files/mesondisplay.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/mesondisplay.cfg

ifeq ($(USE_XML_AUDIO_POLICY_CONF), 1)
PRODUCT_COPY_FILES += \
    device/amlogic/$(PRODUCT_DIR)/files/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    device/amlogic/$(PRODUCT_DIR)/files/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml
else
PRODUCT_COPY_FILES += \
    device/amlogic/$(PRODUCT_DIR)/files/audio_policy.conf:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy.conf
endif

ifeq ($(TARGET_WITH_MEDIA_EXT), true)
PRODUCT_COPY_FILES += \
    device/amlogic/$(PRODUCT_DIR)/files/media_codecs_ext.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_ext.xml
endif

# remote IME config file
PRODUCT_COPY_FILES += \
    device/amlogic/$(PRODUCT_DIR)/files/remote.conf:$(TARGET_COPY_OUT_VENDOR)/etc/remote.conf \
    device/amlogic/$(PRODUCT_DIR)/files/remote.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/remote.cfg \
    device/amlogic/$(PRODUCT_DIR)/files/remote.tab:$(TARGET_COPY_OUT_VENDOR)/etc/remote.tab \
    device/amlogic/common/products/tv/Vendor_0001_Product_0001.kl:/$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/Vendor_0001_Product_0001.kl \
    device/amlogic/common/products/tv/Vendor_1915_Product_0001.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/Vendor_1915_Product_0001.kl
ifneq ($(TARGET_BUILD_GOOGLE_ATV), true)
PRODUCT_COPY_FILES += \
    device/amlogic/$(PRODUCT_DIR)/files/Generic.kl:/$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/Generic.kl
else
PRODUCT_COPY_FILES += \
    device/amlogic/common/Generic.kl:/$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/Generic.kl
endif
# recovery
PRODUCT_COPY_FILES += \
    device/amlogic/$(PRODUCT_DIR)/recovery/init.recovery.amlogic.rc:root/init.recovery.amlogic.rc \
    device/amlogic/$(PRODUCT_DIR)/recovery/recovery.kl:recovery/root/etc/recovery.kl \
    device/amlogic/$(PRODUCT_DIR)/files/mesondisplay.cfg:recovery/root/etc/mesondisplay.cfg \
    device/amlogic/common/recovery/busybox:recovery/root/sbin/busybox \
    device/amlogic/$(PRODUCT_DIR)/recovery/remotecfg:recovery/root/sbin/remotecfg \
    device/amlogic/$(PRODUCT_DIR)/files/remote.cfg:recovery/root/etc/remote.cfg \
    device/amlogic/$(PRODUCT_DIR)/files/remote.tab:recovery/root/etc/remote.tab \
    device/amlogic/common/recovery/resize2fs:recovery/root/sbin/resize2fs \
    device/amlogic/$(PRODUCT_DIR)/recovery/sh:recovery/root/sbin/sh

# lyell config file
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,device/amlogic/$(PRODUCT_DIR)/files/tv/tvconfig/,/$(TARGET_COPY_OUT_VENDOR)/etc/tvconfig) \
    device/amlogic/$(PRODUCT_DIR)/files/tv/dec:$(TARGET_COPY_OUT_VENDOR)/bin/dec \
    device/amlogic/$(PRODUCT_DIR)/files/PQ/pq.db:$(TARGET_COPY_OUT_VENDOR)/etc/tvconfig/pq/pq.db \
    device/amlogic/$(PRODUCT_DIR)/files/PQ/overscan.db:$(TARGET_COPY_OUT_VENDOR)/etc/tvconfig/pq/overscan.db \
    device/amlogic/$(PRODUCT_DIR)/files/PQ/pq_default.ini:$(TARGET_COPY_OUT_VENDOR)/etc/tvconfig/pq/pq_default.ini

#DNLP ko
ifeq ($(KERNEL_A32_SUPPORT), true)
PRODUCT_COPY_FILES += \
    device/amlogic/common/video_algorithm/dnlp/dnlp_alg_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/dnlp_alg.ko
else
PRODUCT_COPY_FILES += \
    device/amlogic/common/video_algorithm/dnlp/dnlp_alg_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/dnlp_alg.ko
endif

PRODUCT_AAPT_CONFIG := xlarge hdpi xhdpi tvdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi

PRODUCT_CHARACTERISTICS := lyell,nosdcard
ifneq ($(TARGET_BUILD_GOOGLE_ATV), true)
DEVICE_PACKAGE_OVERLAYS := \
    device/amlogic/$(PRODUCT_DIR)/overlay
endif
PRODUCT_TAGS += dalvik.gc.type-precise


# setup dalvik vm configs.
$(call inherit-product, frameworks/native/build/tablet-7in-hdpi-1024-dalvik-heap.mk)






#To remove healthd from the build
PRODUCT_PACKAGES += android.hardware.health@2.0-service.override
DEVICE_FRAMEWORK_MANIFEST_FILE += \
	system/libhidl/vintfdata/manifest_healthd_exclude.xml

#To keep healthd in the build
PRODUCT_PACKAGES += android.hardware.health@2.0-service
