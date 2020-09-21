#
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

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := RSTest_Compat
LOCAL_SDK_VERSION := current
LOCAL_MODULE_TAGS := tests

LOCAL_SRC_FILES := \
    $(call all-java-files-under,src) \
    $(call all-java-files-under,../RSUnitTests/gui) \
    $(call all-java-files-under,../RSUnitTests/supportlibsrc_gen) \
    $(call all-renderscript-files-under,../RSUnitTests/supportlibsrc_gen) \

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-test \
    android-support-v8-renderscript \

LOCAL_RENDERSCRIPT_TARGET_API := 0
LOCAL_RENDERSCRIPT_COMPATIBILITY := true
LOCAL_MIN_SDK_VERSION := 8

LOCAL_RENDERSCRIPT_FLAGS := -rs-package-name=android.support.v8.renderscript

include $(BUILD_PACKAGE)

