# Copyright (C) 2017 The Android Open Source Project
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

ifneq ($(TARGET_BUILD_PDK), true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# To avoid build errors, build empty package for non-platform builds
# (for example, projected). See b/30064991
ifeq (,$(TARGET_BUILD_APPS))
  LOCAL_PACKAGE_NAME := CarSettings
  LOCAL_PRIVATE_PLATFORM_APIS := true

  LOCAL_SRC_FILES := $(call all-java-files-under, src)

  LOCAL_USE_AAPT2 := true

  LOCAL_JAVA_LIBRARIES += android.car

  LOCAL_STATIC_ANDROID_LIBRARIES := \
      android-support-car \
      android-support-v7-preference \
      android-support-v14-preference \
      android-arch-lifecycle-extensions \
      car-list \
      car-settings-lib \
      setup-wizard-lib-gingerbread-compat \
      SettingsLib

  LOCAL_RESOURCE_DIR := \
      $(LOCAL_PATH)/res

  LOCAL_CERTIFICATE := platform

  LOCAL_MODULE_TAGS := optional

  LOCAL_PROGUARD_ENABLED := disabled

  LOCAL_PRIVILEGED_MODULE := true

  LOCAL_DEX_PREOPT := false

  LOCAL_STATIC_JAVA_LIBRARIES += jsr305

  LOCAL_DX_FLAGS := --multi-dex

  ifdef DISABLE_AOSP_PHONE_SETTING
    ifeq ($(DISABLE_AOSP_PHONE_SETTING),true)
      # This will hide AOSP phone setting.
      LOCAL_OVERRIDES_PACKAGES := Settings
    endif
  endif
  include $(BUILD_PACKAGE)
endif

# Use the following include to make our test apk.
ifeq (,$(ONE_SHOT_MAKEFILE))
include $(call first-makefiles-under, $(LOCAL_PATH))
endif

endif
