#
# Copyright (C) 2016 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res

LOCAL_PACKAGE_NAME := CtsNoRestartFeature
LOCAL_SDK_VERSION := current

LOCAL_MODULE_TAGS := tests
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests cts_instant

LOCAL_PROGUARD_ENABLED := disabled
LOCAL_DEX_PREOPT := false

localRStamp := $(call intermediates-dir-for,APPS,$(LOCAL_PACKAGE_NAME),,COMMON)/src/R.stamp
featureOf := CtsNoRestartBase
featureOfApk := $(call intermediates-dir-for,APPS,$(featureOf))/package.apk
$(localRStamp): $(featureOfApk)
LOCAL_APK_LIBRARIES := $(featureOf)
LOCAL_AAPT_FLAGS += --feature-of $(featureOfApk)

include $(BUILD_CTS_SUPPORT_PACKAGE)
