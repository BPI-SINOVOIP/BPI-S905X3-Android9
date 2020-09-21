# Copyright (C) 2018 The Android Open Source Project
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
LOCAL_MODULE := CtsCorruptApkTests_b71360999
LOCAL_MODULE_CLASS := APPS
LOCAL_SRC_FILES := b71360999.apk
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests
LOCAL_CERTIFICATE := PRESIGNED
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := CtsCorruptApkTests_b71361168
LOCAL_MODULE_CLASS := APPS
LOCAL_SRC_FILES := b71361168.apk
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests
LOCAL_CERTIFICATE := PRESIGNED
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := CtsCorruptApkTests_b79488511
LOCAL_MODULE_CLASS := APPS
LOCAL_SRC_FILES := b79488511.apk
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests
LOCAL_CERTIFICATE := PRESIGNED
include $(BUILD_PREBUILT)