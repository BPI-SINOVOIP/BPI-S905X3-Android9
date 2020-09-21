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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := \
    compatibility-device-util \
    android-support-test \
    junit

LOCAL_JAVA_LIBRARIES := \
    android.test.base.stubs \
    framework-stub-for-compatibility-device-info

LOCAL_MODULE := compatibility-device-info

LOCAL_SDK_VERSION := current

include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
# This stub library provides some internal APIs (SystemProperties, VintfObject, etc.)
# to compatibility-device-info library.
LOCAL_MODULE := framework-stub-for-compatibility-device-info
LOCAL_SRC_FILES := $(call all-java-files-under, src_stub)
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := current
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_JAVA_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
