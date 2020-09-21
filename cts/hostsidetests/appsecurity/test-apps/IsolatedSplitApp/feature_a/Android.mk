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

LOCAL_PACKAGE_NAME := CtsIsolatedSplitAppFeatureA
LOCAL_SDK_VERSION := current
LOCAL_USE_AAPT2 := true
LOCAL_MODULE_TAGS := tests
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests

# Feature splits are dependent on this split, so it must be exported.
LOCAL_EXPORT_PACKAGE_RESOURCES := true

# Make sure our test locale polish is not stripped.
LOCAL_AAPT_INCLUDE_ALL_RESOURCES := true

LOCAL_SRC_FILES := $(call all-subdir-java-files)

# Generate a locale split.
LOCAL_PACKAGE_SPLITS := pl

# Code and resource dependency on the base.
LOCAL_APK_LIBRARIES := CtsIsolatedSplitApp
LOCAL_RES_LIBRARIES := $(LOCAL_APK_LIBRARIES)

# Although feature splits use unique resource package names, they must all
# have the same manifest package name to be considered one app.
LOCAL_AAPT_FLAGS += --rename-manifest-package com.android.cts.isolatedsplitapp

# Assign a unique package ID to this feature split. Since these are isolated splits,
# it must only be unique across a dependency chain.
LOCAL_AAPT_FLAGS += --package-id 0x80

include $(BUILD_CTS_SUPPORT_PACKAGE)
