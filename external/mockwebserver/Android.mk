#
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
#
LOCAL_PATH := $(call my-dir)

mockwebserver_src_files := $(call all-java-files-under,src/main/java)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(mockwebserver_src_files)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mockwebserver
# Some tests (CtsVerifier, etc) that are built with SDK are using this library,
# thus this lib should be built with public APIs. Since this lib is not specific
# to Android, core_current which is a core-Java subset of Android SDK is used.
LOCAL_SDK_VERSION := core_current
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(mockwebserver_src_files)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mockwebserver-host
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
include $(BUILD_HOST_DALVIK_STATIC_JAVA_LIBRARY)
