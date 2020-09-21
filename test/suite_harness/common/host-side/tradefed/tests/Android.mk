# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Make a mock compatibility suite to test
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include cts/error_prone_rules.mk

LOCAL_SUITE_BUILD_NUMBER := 2
LOCAL_SUITE_TARGET_ARCH := $(TARGET_ARCH)
LOCAL_SUITE_NAME := TESTS
LOCAL_SUITE_FULLNAME := "Compatibility Tests"
LOCAL_SUITE_VERSION := 1

LOCAL_MODULE := compatibility-mock-tradefed
LOCAL_STATIC_JAVA_LIBRARIES := cts-tradefed-harness
include cts/error_prone_rules.mk
include $(BUILD_COMPATIBILITY_SUITE)

# Make the tests
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_JAVA_RESOURCE_DIRS := res

include cts/error_prone_rules.mk
LOCAL_MODULE := compatibility-tradefed-tests

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := easymock

LOCAL_JAVA_LIBRARIES := tradefed compatibility-mock-tradefed junit-host compatibility-host-util

include $(BUILD_HOST_JAVA_LIBRARY)
