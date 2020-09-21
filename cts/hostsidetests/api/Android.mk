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

LOCAL_MODULE := CtsUnofficialApisUsageTestCases
LOCAL_MODULE_TAGS := tests
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_SDK_VERSION := current
LOCAL_JAVA_LIBRARIES := cts-tradefed tradefed compatibility-host-util
LOCAL_STATIC_JAVA_LIBRARIES := dexlib2 doclava jsilver guavalib antlr-runtime host-jdk-tools-prebuilt \
    compatibility-host-util

# These are list of api txt files that are considered as approved APIs
LOCAL_JAVA_RESOURCE_FILES := $(addprefix frameworks/base/,\
api/current.txt \
api/system-current.txt \
test-base/api/android-test-base-current.txt \
test-runner/api/android-test-runner-current.txt \
test-mock/api/android-test-mock-current.txt)

# API 27 is added since some support libraries are using old APIs
LOCAL_JAVA_RESOURCE_FILES += prebuilts/sdk/api/27.txt

include $(BUILD_HOST_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := host-jdk-tools-prebuilt:../../../$(HOST_JDK_TOOLS_JAR)
include $(BUILD_HOST_PREBUILT)
