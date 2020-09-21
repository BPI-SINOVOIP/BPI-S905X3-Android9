#
# Copyright (C) 2008 The Android Open Source Project
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
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests cts_instant

# Include both the 32 and 64 bit versions
LOCAL_MULTILIB := both

LOCAL_JAVA_LIBRARIES := telephony-common

LOCAL_STATIC_JAVA_LIBRARIES := \
    ctstestrunner \
    guava \
    android-ex-camera2 \
    compatibility-device-util

LOCAL_JNI_SHARED_LIBRARIES := libctspermission_jni libnativehelper_compat_libc++

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := CtsPermissionTestCases

# uncomment when b/13249777 is fixed
#LOCAL_SDK_VERSION := current
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_JAVA_LIBRARIES += android.test.runner.stubs
LOCAL_JAVA_LIBRARIES += android.test.base.stubs

include $(BUILD_CTS_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
