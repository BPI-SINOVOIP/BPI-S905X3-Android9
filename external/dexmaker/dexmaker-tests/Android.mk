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
#
#

LOCAL_PATH := $(call my-dir)

# Build Dexmaker's tests
#
# Run the tests as follows:
#   vogar --classpath ${ANDROID_PRODUCT_OUT}/obj/JAVA_LIBRARIES/dexmaker-tests_intermediates/javalib.jar \
        com.android.dx

include $(CLEAR_VARS)
LOCAL_MODULE := dexmaker-tests
LOCAL_SDK_VERSION := 17
LOCAL_SRC_FILES := $(call all-java-files-under, src/androidTest/java)
LOCAL_STATIC_JAVA_LIBRARIES := dexmaker android-support-test
LOCAL_ERROR_PRONE_FLAGS := -Xep:JUnit4TestNotRun:WARN
include $(BUILD_STATIC_JAVA_LIBRARY)

# Build a test APK
#
# Run the tests as follows:
# m -j32 DexmakerTests && \
        am install -r -g $OUT/data/app/DexmakerTests/DexmakerTests.apk \
        adb shell am instrument -w com.google.dexmaker.tests
#
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_PACKAGE_NAME := DexmakerTests
LOCAL_SDK_VERSION := current
LOCAL_STATIC_JAVA_LIBRARIES := \
        dexmaker-tests

include $(BUILD_PACKAGE)
