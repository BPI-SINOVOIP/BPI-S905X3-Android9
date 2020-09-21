#
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
#

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_USE_AAPT2 := true
LOCAL_MODULE_TAGS := tests
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests
LOCAL_EXPORT_PACKAGE_RESOURCES := true
LOCAL_PACKAGE_NAME := CtsClassloaderSplitAppFeatureA
LOCAL_SDK_VERSION := current

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_APK_LIBRARIES := CtsClassloaderSplitApp
LOCAL_RES_LIBRARIES := $(LOCAL_APK_LIBRARIES)

LOCAL_AAPT_FLAGS += --custom-package com.android.cts.classloadersplitapp.feature_a
LOCAL_AAPT_FLAGS += --package-id 0x80

include $(BUILD_CTS_SUPPORT_PACKAGE)
