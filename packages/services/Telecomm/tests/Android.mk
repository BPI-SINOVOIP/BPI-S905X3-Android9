#
# Copyright (C) 2013 The Android Open Source Project
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

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-ex-camera2 \
    guava \
    mockito-target \
    android-support-test \
    platform-test-annotations

LOCAL_STATIC_ANDROID_LIBRARIES := \
    android-support-core-ui \
    android-support-core-utils \
    android-support-compat \
    android-support-fragment

LOCAL_SRC_FILES := \
    $(call all-java-files-under, src) \
    $(call all-java-files-under, ../src) \
    $(call all-proto-files-under, ../proto)

LOCAL_PROTOC_OPTIMIZE_TYPE := nano
LOCAL_PROTOC_FLAGS := --proto_path=$(LOCAL_PATH)/../proto/
LOCAL_PROTO_JAVA_OUTPUT_PARAMS := optional_field_style=accessors

LOCAL_RESOURCE_DIR := \
    $(LOCAL_PATH)/res \
    $(LOCAL_PATH)/../res

LOCAL_JAVA_LIBRARIES := \
    android.test.mock \
    android.test.base \
    android.test.runner \
    telephony-common

LOCAL_USE_AAPT2 := true

LOCAL_AAPT_FLAGS := \
    --auto-add-overlay \
    --extra-packages com.android.server.telecom

LOCAL_JACK_FLAGS := --multi-dex native

LOCAL_PROGUARD_ENABLED := disabled

LOCAL_PACKAGE_NAME := TelecomUnitTests
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_CERTIFICATE := platform

LOCAL_MODULE_TAGS := tests

LOCAL_JACK_COVERAGE_INCLUDE_FILTER := com.android.server.telecom.*
LOCAL_JACK_COVERAGE_EXCLUDE_FILTER := com.android.server.telecom.tests.*

LOCAL_COMPATIBILITY_SUITE := device-tests
include frameworks/base/packages/SettingsLib/common.mk

include $(BUILD_PACKAGE)
