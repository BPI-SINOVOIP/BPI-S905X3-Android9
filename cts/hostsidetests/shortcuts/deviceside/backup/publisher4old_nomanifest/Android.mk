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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests

LOCAL_PACKAGE_NAME := CtsShortcutBackupPublisher4old_nomanifest

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_SRC_FILES := $(call all-java-files-under, ../publisher4old/src) \
    $(call all-java-files-under, ../../common/src)

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/../publisher4old/res

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-test \
    androidx.legacy_legacy-support-v4 \
    mockito-target-minus-junit4 \
    compatibility-device-util \
    ctstestrunner \
    ub-uiautomator \
    ShortcutManagerTestUtils

LOCAL_JAVA_LIBRARIES := android.test.base.stubs

LOCAL_SDK_VERSION := current

include $(BUILD_CTS_PACKAGE)
