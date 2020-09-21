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

#
# Reusable Sensor test classes and helpers
#
include $(CLEAR_VARS)

LOCAL_MODULE := cts-sensors-tests

LOCAL_MODULE_TAGS := tests

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_STATIC_JAVA_LIBRARIES := compatibility-device-util

LOCAL_JAVA_LIBRARIES := platform-test-annotations android.test.base.stubs

LOCAL_SDK_VERSION := current

LOCAL_SRC_FILES := $(call all-java-files-under, src)
-include cts/error_prone_rules_tests.mk
include $(BUILD_STATIC_JAVA_LIBRARY)

#
# JNI components for testing NDK
#
include $(CLEAR_VARS)

LOCAL_MODULE := libcts-sensors-ndk-jni

LOCAL_CFLAGS += -Werror -Wall -Wextra

LOCAL_MODULE_TAGS := tests

LOCAL_SRC_FILES := \
    jni/SensorTest.cpp \
    jni/SensorTestCases.cpp \
    jni/android_hardware_cts_SensorDirectReportTest.cpp \
    jni/android_hardware_cts_SensorNativeTest.cpp \
    jni/nativeTestHelper.cpp

LOCAL_C_INCLUDES := $(JNI_H_INCLUDE)

LOCAL_SHARED_LIBRARIES := libandroid liblog

LOCAL_SDK_VERSION := current

LOCAL_NDK_STL_VARIANT := c++_shared
-include cts/error_prone_rules_tests.mk
include $(BUILD_SHARED_LIBRARY)

#
# CtsSensorTestCases package
#
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests

# include both the 32 and 64 bit versions
LOCAL_MULTILIB := both

LOCAL_STATIC_JAVA_LIBRARIES := \
    compatibility-device-util \
    ctstestrunner \
    cts-sensors-tests \

LOCAL_JNI_SHARED_LIBRARIES := libcts-sensors-ndk-jni

LOCAL_PACKAGE_NAME := CtsSensorTestCases

LOCAL_SDK_VERSION := current

LOCAL_JAVA_LIBRARIES := android.test.runner.stubs android.test.base.stubs

LOCAL_NDK_STL_VARIANT := c++_shared

include $(BUILD_CTS_PACKAGE)
