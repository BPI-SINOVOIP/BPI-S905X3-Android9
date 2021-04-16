# Copyright (C) 2014 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_PACKAGE_NAME := DroidTvSettings

LOCAL_CERTIFICATE := platform
LOCAL_MODULE_TAGS := optional
LOCAL_PROGUARD_FLAG_FILES := proguard.cfg
LOCAL_USE_AAPT2 := true
LOCAL_PRIVILEGED_MODULE := true
LOCAL_PRIVATE_PLATFORM_APIS := true

LOCAL_JAVA_LIBRARIES := droidlogic droidlogic-tv
#include frameworks/base/packages/SettingsLib/common.mk

LOCAL_STATIC_ANDROID_LIBRARIES := \
    android-support-v7-recyclerview \
    android-support-v7-preference \
    android-support-v7-appcompat \
    android-support-v14-preference \
    android-support-v17-preference-leanback \
    android-support-v17-leanback \
    android-arch-lifecycle-extensions

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-arch-lifecycle-common-java8

LOCAL_RESOURCE_DIR := \
    $(LOCAL_PATH)/res

LOCAL_SRC_FILES := \
    $(call all-java-files-under, src) \
    $(call all-Iaidl-files-under, src)
#include frameworks/opt/setupwizard/library/common-gingerbread.mk
#include frameworks/base/packages/SettingsLib/common.mk


FILE := device/*/$(TARGET_PRODUCT)/files/DroidTvSettings/AndroidManifest-common.xml
FILES := $(foreach v,$(wildcard $(FILE)),$(v))

ifeq ($(FILES), $(wildcard $(FILE)))
LOCAL_FULL_LIBS_MANIFEST_FILES := $(FILES)
LOCAL_JACK_COVERAGE_INCLUDE_FILTER := com.droidlogic.tv.settings*
endif

include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
