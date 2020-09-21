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
LOCAL_MODULE := app-helpers-common-interfaces
LOCAL_JAVA_LIBRARIES := ub-uiautomator
LOCAL_STATIC_JAVA_LIBRARIES := app-helpers-core
LOCAL_SRC_FILES := $(call all-java-files-under, common)
LOCAL_SDK_VERSION := current

include $(BUILD_STATIC_JAVA_LIBRARY)

######################################

include $(CLEAR_VARS)
LOCAL_MODULE := app-helpers-auto-interfaces
LOCAL_JAVA_LIBRARIES := ub-uiautomator app-helpers-core
LOCAL_STATIC_JAVA_LIBRARIES := app-helpers-common-interfaces
LOCAL_SRC_FILES := $(call all-java-files-under, auto)
LOCAL_SDK_VERSION := current

include $(BUILD_STATIC_JAVA_LIBRARY)

######################################

include $(CLEAR_VARS)
LOCAL_MODULE := app-helpers-clockwork-interfaces
LOCAL_JAVA_LIBRARIES := ub-uiautomator app-helpers-core
LOCAL_STATIC_JAVA_LIBRARIES := app-helpers-common-interfaces
LOCAL_SRC_FILES := $(call all-java-files-under, clockwork)
LOCAL_SDK_VERSION := current

include $(BUILD_STATIC_JAVA_LIBRARY)

######################################

include $(CLEAR_VARS)
LOCAL_MODULE := app-helpers-handheld-interfaces
LOCAL_JAVA_LIBRARIES := ub-uiautomator app-helpers-core
LOCAL_STATIC_JAVA_LIBRARIES := app-helpers-common-interfaces
LOCAL_SRC_FILES := $(call all-java-files-under, handheld)
LOCAL_SDK_VERSION := current

include $(BUILD_STATIC_JAVA_LIBRARY)

######################################

include $(CLEAR_VARS)
LOCAL_MODULE := app-helpers-tv-interfaces
LOCAL_JAVA_LIBRARIES := ub-uiautomator app-helpers-core launcher-helper-lib
LOCAL_STATIC_JAVA_LIBRARIES := app-helpers-common-interfaces dpad-util
LOCAL_SRC_FILES := $(call all-java-files-under, tv)
LOCAL_SDK_VERSION := current

include $(BUILD_STATIC_JAVA_LIBRARY)
