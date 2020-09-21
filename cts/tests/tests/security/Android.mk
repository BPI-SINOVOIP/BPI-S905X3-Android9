# Copyright (C) 2011 The Android Open Source Project
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

LOCAL_MODULE_TAGS := tests

# Include both the 32 and 64 bit versions
LOCAL_MULTILIB := both

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-test \
    android-common \
    ctstestserver \
    ctstestrunner \
    compatibility-device-util \
    guava \
    platform-test-annotations

LOCAL_JAVA_LIBRARIES := \
    android.test.runner.stubs \
    org.apache.http.legacy \
    android.test.base.stubs \


LOCAL_JNI_SHARED_LIBRARIES := libctssecurity_jni libcts_jni libnativehelper_compat_libc++ \
                      libnativehelper \
                      libcutils \
                      libcrypto \
                      libselinux \
                      libc++ \
                      libpcre2 \
                      libpackagelistparser

LOCAL_SRC_FILES := $(call all-java-files-under, src)\
                   src/android/security/cts/activity/ISecureRandomService.aidl\
                   aidl/android/security/cts/IIsolatedService.aidl

LOCAL_PACKAGE_NAME := CtsSecurityTestCases

#LOCAL_SDK_VERSION := current
LOCAL_PRIVATE_PLATFORM_APIS := true

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests sts

include $(BUILD_CTS_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
