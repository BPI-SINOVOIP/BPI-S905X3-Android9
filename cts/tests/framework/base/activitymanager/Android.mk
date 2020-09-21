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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests optional

# Must match the package name in CtsTestCaseList.mk
LOCAL_PACKAGE_NAME := CtsActivityManagerDeviceTestCases
LOCAL_PRIVATE_PLATFORM_APIS := true

LOCAL_SRC_FILES := \
    $(call all-java-files-under, src) \
    $(call all-named-files-under,Components.java, app) \
    $(call all-named-files-under,Components.java, app27) \
    $(call all-named-files-under,Components.java, appDebuggable) \
    $(call all-named-files-under,Components.java, appDeprecatedSdk) \
    $(call all-named-files-under,Components.java, appDisplaySize) \
    $(call all-named-files-under,Components.java, appPrereleaseSdk) \
    $(call all-named-files-under,Components.java, appSecondUid) \
    $(call all-named-files-under,Components.java, appThirdUid) \
    $(call all-named-files-under,Components.java, translucentapp) \
    $(call all-named-files-under,Components.java, translucentappsdk26) \

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-test \
    cts-amwm-util

LOCAL_CTS_TEST_PACKAGE := android.server

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests

include $(BUILD_CTS_PACKAGE)

# Build the test APKs using their own makefiles
include $(call all-makefiles-under,$(LOCAL_PATH))
