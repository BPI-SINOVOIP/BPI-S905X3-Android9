# Copyright (C) 2011 Amlogic Inc
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

# Set display related config
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.platform.has.mbxuimode=true \
    ro.vendor.platform.has.realoutputmode=true \
    ro.vendor.platform.need.display.hdmicec=true

#camera max to 720p
#PRODUCT_PROPERTY_OVERRIDES += \
    #ro.media.camera_preview.maxsize=1280x720 \
    #ro.media.camera_preview.limitedrate=1280x720x30,640x480x30,320x240x28

#camera max to 1080p
PRODUCT_PROPERTY_OVERRIDES += \
    ro.media.camera_preview.maxsize=1920x1080 \
    ro.media.camera_preview.limitedrate=1920x1080x30,1280x720x30,640x480x30,320x240x28 \
    ro.media.camera_preview.usemjpeg=1

#if wifi Only
PRODUCT_PROPERTY_OVERRIDES +=  \
    ro.radio.noril=false

#if need pppoe
PRODUCT_PROPERTY_OVERRIDES +=  \
    ro.net.pppoe=true

PRODUCT_PROPERTY_OVERRIDES += \
   ro.vendor.platform.support.dolbyvision=true

#the prop is used for enable or disable
#DD+/DD force output when HDMI EDID is not supported
#by default,the force output mode is enabled.
#Note,please do not set the prop to true by default
#only for netflix,just disable the feature.so set the prop to true
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.platform.disable.audiorawout=false

#Dolby DD+ decoder option
#this prop to for videoplayer display the DD+/DD icon when playback
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.platform.support.dolby=true
#DTS decoder option
#display dts icon when playback
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.platform.support.dts=true
#DTS-HD support prop
#PRODUCT_PROPERTY_OVERRIDES += \
    #ro.vendor.platform.support.dtstrans=true \
    #ro.vendor.platform.support.dtsmulasset=true
#DTS-HD prop end
# Enable player buildin

PRODUCT_PROPERTY_OVERRIDES += \
    media.support.dolbyvision = true

#add for video boot, 1 means use video boot, others not .
PRODUCT_PROPERTY_OVERRIDES +=  \
    service.bootvideo=0

# Define drm for this device
PRODUCT_PROPERTY_OVERRIDES +=  \
    drm.service.enabled=1

#set memory upper limit for extractor process
PRODUCT_PROPERTY_OVERRIDES += \
    ro.media.maxmem=629145600

#map volume
PRODUCT_PROPERTY_OVERRIDES +=  \
    ro.audio.mapvalue=0,0,0,0

#adb
PRODUCT_PROPERTY_OVERRIDES +=  \
    service.adb.tcp.port=5555

# crypto volume
PRODUCT_PROPERTY_OVERRIDES +=  \
    ro.crypto.volume.filenames_mode=aes-256-cts


# Low memory platform
PRODUCT_PROPERTY_OVERRIDES +=  \
    ro.config.low_ram=true \
    ro.platform.support.dolbyvision=true

#enable/disable afbc
PRODUCT_PROPERTY_OVERRIDES +=  \
    vendor.afbcd.enable=1

# default enable sdr to hdr
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.sdr2hdr.enable=true

PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.platform.is.tv=0

#bootvideo
#0                      |050
#^                      |
#|                      |
#0:bootanim             |
#1:bootanim + bootvideo |
#2:bootvideo + bootanim |
#3:bootvideo            |
#others:bootanim        |
#-----------------------|050
#050:default volume value, volume range 0~100
#note that the high position 0 can not be omitted
ifneq ($(TARGET_BUILD_GOOGLE_ATV), true)
PRODUCT_PROPERTY_OVERRIDES += \
    persist.vendor.media.bootvideo=0050
endif

PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.platform.hdmi.device_type=4
