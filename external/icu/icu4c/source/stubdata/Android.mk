# Copyright (C) 2008 The Android Open Source Project
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
# Common definitions for all variants.
#

LOCAL_PATH:= $(call my-dir)

# Derive a string like 'icudt 48l.dat' from a path like 'external/icu/icu4c/source/stubdata/icudt 48l.dat'
stubdata_path:= $(call my-dir)
dat_file := $(notdir $(wildcard $(stubdata_path)/*.dat))

include $(CLEAR_VARS)
LOCAL_MODULE := icu-data
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/icu
LOCAL_MODULE_STEM := $(dat_file)
LOCAL_SRC_FILES := $(dat_file)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := icu-data
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/usr/icu
LOCAL_MODULE_STEM := $(dat_file)
LOCAL_SRC_FILES := $(dat_file)
LOCAL_IS_HOST_MODULE := true
include $(BUILD_PREBUILT)
