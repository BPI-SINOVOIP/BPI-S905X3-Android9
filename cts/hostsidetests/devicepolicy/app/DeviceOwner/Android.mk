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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := CtsDeviceOwnerApp
LOCAL_PRIVATE_PLATFORM_APIS := true

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_SRC_FILES := $(call all-java-files-under, src) \
                   $(call all-Iaidl-files-under, src)

LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/src

LOCAL_JAVA_LIBRARIES := \
    android.test.runner.stubs \
    cts-junit \
    android.test.base.stubs \
    bouncycastle

LOCAL_USE_AAPT2 := true

LOCAL_STATIC_JAVA_LIBRARIES := \
    ctstestrunner \
    compatibility-device-util \
    android-support-test \
    cts-security-test-support-library \
    testng

LOCAL_STATIC_ANDROID_LIBRARIES := \
    androidx.legacy_legacy-support-v4

# tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := arcts cts vts general-tests

include $(BUILD_CTS_PACKAGE)
