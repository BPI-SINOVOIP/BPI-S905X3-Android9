# Copyright 2011, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)

src_dirs := src

##################################################

include $(CLEAR_VARS)

# We only want this apk build for tests.
LOCAL_MODULE_TAGS := tests

# LOCAL_JAVA_LIBRARIES := android.test.runner

LOCAL_SDK_VERSION := current
LOCAL_PACKAGE_NAME := UnifiedEmailTests
LOCAL_INSTRUMENTATION_FOR := UnifiedEmail

LOCAL_SRC_FILES := $(call all-java-files-under, $(src_dirs))
LOCAL_AAPT_FLAGS := --auto-add-overlay
LOCAL_STATIC_JAVA_LIBRARIES := android-support-test

LOCAL_JAVA_LIBRARIES := \
    android.test.mock.stubs \
    android.test.runner.stubs \
    android.test.base.stubs \


include $(BUILD_PACKAGE)

# Build all sub-directories
include $(call all-makefiles-under,$(LOCAL_PATH))
