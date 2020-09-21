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


LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

# don't include this package in any target
LOCAL_MODULE_TAGS := optional
# and when built explicitly put it in the data partition
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_STATIC_JAVA_LIBRARIES := \
    ctstestrunner \
    compatibility-device-util \
    junit

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := CtsCarrierApiTestCases
LOCAL_PRIVATE_PLATFORM_APIS := true

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests

LOCAL_JAVA_LIBRARIES += android.test.runner.stubs telephony-common
LOCAL_JAVA_LIBRARIES += android.test.base.stubs

# This APK must be signed to match the test SIM's cert whitelist.
# While "testkey" is the default, there are different per-device testkeys, so
# hard-code the AOSP default key to ensure it is used regardless of build
# environment.
LOCAL_CERTIFICATE := build/make/target/product/security/testkey

include $(BUILD_CTS_PACKAGE)
