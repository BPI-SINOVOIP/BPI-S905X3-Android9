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

LOCAL_PATH:= $(call my-dir)

##################################################################
# Unsigned Package

include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := CtsSecureElementAccessControlTestCases1
# Don't include this package in any target.
LOCAL_MODULE_TAGS := optional
# When built, explicitly put it in the data partition.
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)
LOCAL_STATIC_JAVA_LIBRARIES := \
	ctstestrunner \
	compatibility-device-util
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_SDK_VERSION := current
LOCAL_JAVA_LIBRARIES += android.test.runner
LOCAL_JAVA_LIBRARIES += android.test.base
# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests

include $(BUILD_CTS_PACKAGE)

##################################################################
# Signed Package

include $(CLEAR_VARS)

LOCAL_MODULE := signed-CtsSecureElementAccessControlTestCases1
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE_CLASS := APPS
LOCAL_BUILT_MODULE_STEM := package.apk
# Make sure the build system doesn't try to resign the APK
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests

LOCAL_REPLACE_PREBUILT_APK_INSTALLED := $(LOCAL_PATH)/apk/signed-CtsSecureElementAccessControlTestCases1.apk

include $(BUILD_PREBUILT)
