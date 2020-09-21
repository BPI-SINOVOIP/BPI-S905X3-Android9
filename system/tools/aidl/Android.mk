#
# Copyright (C) 2015 The Android Open Source Project
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

#
# Everything below here is used for integration testing of generated AIDL code.
#

# aidl on its own doesn't need the framework, but testing native/java
# compatibility introduces java dependencies.
ifndef BRILLO

include $(CLEAR_VARS)
LOCAL_PACKAGE_NAME := aidl_test_services
LOCAL_PRIVATE_PLATFORM_APIS := true
# Turn off Java optimization tools to speed up our test iterations.
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_DEX_PREOPT := false
LOCAL_CERTIFICATE := platform
LOCAL_MANIFEST_FILE := tests/java_app/AndroidManifest.xml
LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/tests/java_app/resources
LOCAL_SRC_FILES := \
    tests/android/aidl/tests/INamedCallback.aidl \
    tests/android/aidl/tests/ITestService.aidl \
    tests/java_app/src/android/aidl/tests/NullableTests.java \
    tests/java_app/src/android/aidl/tests/SimpleParcelable.java \
    tests/java_app/src/android/aidl/tests/TestFailException.java \
    tests/java_app/src/android/aidl/tests/TestLogger.java \
    tests/java_app/src/android/aidl/tests/TestServiceClient.java
LOCAL_AIDL_INCLUDES := \
    system/tools/aidl/tests/ \
    frameworks/native/aidl/binder
include $(BUILD_PACKAGE)

endif  # not defined BRILLO
